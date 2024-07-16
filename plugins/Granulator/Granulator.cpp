/*
 * Granulator.cpp - originally AudioFileProcessor.cpp, but edited by Regulus to be a granular synth.
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "Granulator.h"
#include "GranulatorView.h"

#include "InstrumentTrack.h"
#include "PathUtil.h"
#include "SampleLoader.h"
#include "Song.h"

#include "plugin_export.h"

#include <QDomElement>

#include <QDebug>
#include "MixHelpers.h"


namespace lmms
{

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT granulator_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Granulator",
	QT_TRANSLATE_NOOP( "PluginBrowser",
				"Copy of AudioFileProcessor"
				" but with granulation!"),
	"Original author of AudioFileProcessor: Tobias Doerffel <tobydox/at/users.sf.net>, but edited by me",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader( "logo" ),
	"wav,ogg,ds,spx,au,voc,aif,aiff,flac,raw"
#ifdef LMMS_HAVE_SNDFILE_MP3
	",mp3"
#endif
	,
	nullptr,
} ;

}




Granulator::Granulator( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &granulator_plugin_descriptor ),
	m_ampModel( 100, 0, 500, 1, this, tr( "Amplify" ) ),
	m_grainSizeModel( 0.25, 0, 1, 0.0000001f, 1000.0f, this, tr( "Grain size" ) ),
	m_grainPositionModel( 0, 0, 1, 0.0000001f, this, tr( "Grain start position" ) ),
	m_spreadModel( 0, 0, 1, 0.0000001f, this, tr( "Spread" ) ),
	m_numGrainsModel( 1, 1, 16, 1, this, tr( "Number of grains" ) ),
	m_startPointModel( 0, 0, 1, 0.0000001f, this, tr( "Start of sample" ) ),
	m_endPointModel( 1, 0, 1, 0.0000001f, this, tr( "End of sample" ) ),
	m_loopPointModel( 0, 0, 1, 0.0000001f, this, tr( "Loopback point" ) ),
	m_reverseModel( false, this, tr( "Reverse sample" ) ),
	m_loopModel( 0, 0, 2, this, tr( "Loop mode" ) ),
	m_stutterModel( false, this, tr( "Stutter" ) ),
	m_interpolationModel( this, tr( "Interpolation mode" ) ),
	m_nextPlayStartPoint( 0 ),
	m_nextPlayBackwards( false )
{
	connect( &m_reverseModel, SIGNAL( dataChanged() ),
				this, SLOT( reverseModelChanged() ), Qt::DirectConnection );
	connect( &m_ampModel, SIGNAL( dataChanged() ),
				this, SLOT( ampModelChanged() ), Qt::DirectConnection );
	connect( &m_startPointModel, SIGNAL( dataChanged() ),
				this, SLOT( startPointChanged() ), Qt::DirectConnection );
	connect( &m_endPointModel, SIGNAL( dataChanged() ),
				this, SLOT( endPointChanged() ), Qt::DirectConnection );
	connect( &m_grainSizeModel, SIGNAL( dataChanged() ),
				this, SLOT( grainSizeChanged() ), Qt::DirectConnection );
	connect( &m_grainPositionModel, SIGNAL( dataChanged() ),
				this, SLOT( grainPositionChanged() ), Qt::DirectConnection );
	connect( &m_spreadModel, SIGNAL( dataChanged() ),
				this, SLOT( spreadChanged() ), Qt::DirectConnection );
	connect( &m_numGrainsModel, SIGNAL( dataChanged() ),
				this, SLOT( numGrainsChanged() ), Qt::DirectConnection );
	connect( &m_loopPointModel, SIGNAL( dataChanged() ),
				this, SLOT( loopPointChanged() ), Qt::DirectConnection );
	connect( &m_stutterModel, SIGNAL( dataChanged() ),
				this, SLOT( stutterModelChanged() ), Qt::DirectConnection );

//interpolation modes
	m_interpolationModel.addItem( tr( "None" ) );
	m_interpolationModel.addItem( tr( "Linear" ) );
	m_interpolationModel.addItem( tr( "Sinc" ) );
	m_interpolationModel.setValue( 1 );

	pointChanged();
}

int Granulator::getNewGrainStartFrame()
{
	// Max offset range goes from grainPos - spread/2 to grainPos + spread/2
	int spread_frames = m_spreadModel.value() * (m_sample.endFrame() - m_sample.startFrame());
	int random_offset = 0;
	// Make sure not to modulo by 0
	if (spread_frames>0){
		random_offset = rand() % (spread_frames) - spread_frames/2;
	}
	int grain_position_frame = m_sample.startFrame() + m_grainPositionModel.value() * (m_sample.endFrame() - m_sample.startFrame());
	return grain_position_frame + random_offset;
}

bool Granulator::addGrain( NotePlayHandle * _n, SampleFrame* _working_buffer, int grain_index, int grain_size, f_cnt_t grain_offset )
{
	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();
	qDebug() << "offset in addGrain:" << offset;
	qDebug() << "frames in addGrain:" << frames;
	qDebug() << "Working buffer in addGrain:" << _working_buffer;
	qDebug() << "Working buffer + offset in addGrain:" << _working_buffer + offset;
	// First fill the buffer with the whole grain sample
	bool success1 = m_sample.play(_working_buffer + offset,
						&static_cast<Sample::PlaybackState*>(_n->m_pluginData)[grain_index],
						frames, _n->frequency(),
						static_cast<Sample::Loop>(m_loopModel.value()));

	bool success2 = true;
	// is a new grain coming up? If so, then add on the start of the next grain to the end of the buffer.
	int frames_since_press = _n->totalFramesPlayed() + grain_offset;
	int frames_until_new_grain = grain_size - (frames_since_press % grain_size);
	if (frames_until_new_grain<frames) {
		// Time to pick new grain
		static_cast<Sample::PlaybackState*>(_n->m_pluginData)[grain_index].setFrameIndex(getNewGrainStartFrame());
		// Fill the end of the buffer with the new section of sample.
		// This is necessary to maintain the correct spacing of grains.
		success2 = m_sample.play(_working_buffer + offset + frames_until_new_grain,
						&static_cast<Sample::PlaybackState*>(_n->m_pluginData)[grain_index],
						frames - frames_until_new_grain, _n->frequency(),
						static_cast<Sample::Loop>(m_loopModel.value()));
	}
	// Make the grain fade in and out to prevent sharp discontinuities: 0% volume at start and end, but 100% in the middle.
	for (fpp_t f=0; f<frames; ++f){
		float pos_in_grain = static_cast<float>((frames_since_press + f) % grain_size) / grain_size;
		float amp = pos_in_grain < 0.5f? pos_in_grain : 1.0f - pos_in_grain;
		_working_buffer[f] *= amp;
	}
	return success1 && success2;
}


void Granulator::playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer )
{
	qDebug() << "Working buffer at start:" << _working_buffer;

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();
	qDebug() << "offset at start:" << offset;

	// Magic key - a frequency < 20 (say, the bottom piano note if using
	// a A4 base tuning) restarts the start point. The note is not actually
	// played.
	if( m_stutterModel.value() == true && _n->frequency() < 20.0 )
	{
		m_nextPlayStartPoint = m_sample.startFrame();
		m_nextPlayBackwards = false;
		return;
	}

	if( !_n->m_pluginData )
	{
		if (m_stutterModel.value() == true && m_nextPlayStartPoint >= m_sample.endFrame())
		{
			// Restart playing the note if in stutter mode, not in loop mode,
			// and we're at the end of the sample.
			m_nextPlayStartPoint = m_sample.startFrame();
			m_nextPlayBackwards = false;
		}
		// set interpolation mode for libsamplerate
		int srcmode = SRC_LINEAR;
		switch( m_interpolationModel.value() )
		{
			case 0:
				srcmode = SRC_ZERO_ORDER_HOLD;
				break;
			case 1:
				srcmode = SRC_LINEAR;
				break;
			case 2:
				srcmode = SRC_SINC_MEDIUM_QUALITY;
				break;
		}
		// Initialize the PlaybackStates, one for each grain
		// Max grain number is 16
		Sample::PlaybackState* temp_array = new Sample::PlaybackState[16];
		for (int g=0; g<16; ++g)
		{
			temp_array[g].setFrameIndex(getNewGrainStartFrame());
			temp_array[g].setBackwards(m_nextPlayBackwards);
		}
		_n->m_pluginData = temp_array;
	}

	if( ! _n->isFinished() )
	{
		int grain_size = m_grainSizeModel.value() * m_sample.sampleRate();
		// Disallow 0 grain size
		grain_size = grain_size == 0? 1 : grain_size;

		bool success = true;
		int num_grains = m_numGrainsModel.value();
		for (int g = 0; g < num_grains; g++)
		{
			SampleFrame* temporary_buffer = new SampleFrame[256];
			success = success && addGrain(_n, temporary_buffer, g, grain_size, static_cast<float>(g)/num_grains * grain_size);
			qDebug() << "Working buffer:" << _working_buffer;
			qDebug() << "Temporary buffer:" << temporary_buffer;
			qDebug() << "Temporary buffer[0]:" << temporary_buffer[0].left() << temporary_buffer[0].right();
			qDebug() << "frames:" << frames;
			MixHelpers::add(_working_buffer, temporary_buffer, frames);
		}
		// Normalize the volume of the output buffer. TODO: This doesn't sound right for some reason; I may be misunderstanding how volume works...?
		MixHelpers::multiply(_working_buffer, 1.0f/num_grains, frames);
		
		if (success)
		{

			applyRelease( _working_buffer, _n );
			emit isPlaying(static_cast<Sample::PlaybackState*>(_n->m_pluginData)[0].frameIndex());
		}
		else
		{
			qDebug() << "Failed???!!!";
			zeroSampleFrames(_working_buffer, frames + offset);
			emit isPlaying( 0 );
		}
	}
	else
	{
		emit isPlaying( 0 );
	}
	if( m_stutterModel.value() == true )
	{
		m_nextPlayStartPoint = static_cast<Sample::PlaybackState*>(_n->m_pluginData)[0].frameIndex();
		m_nextPlayBackwards = static_cast<Sample::PlaybackState*>(_n->m_pluginData)[0].backwards();
	}
}




void Granulator::deleteNotePluginData( NotePlayHandle * _n )
{
	delete [] static_cast<Sample::PlaybackState*>(_n->m_pluginData);
}




void Granulator::saveSettings(QDomDocument& doc, QDomElement& elem)
{
	elem.setAttribute("src", m_sample.sampleFile());
	if (m_sample.sampleFile().isEmpty())
	{
		elem.setAttribute("sampledata", m_sample.toBase64());
	}
	m_reverseModel.saveSettings(doc, elem, "reversed");
	m_loopModel.saveSettings(doc, elem, "looped");
	m_ampModel.saveSettings(doc, elem, "amp");
	m_startPointModel.saveSettings(doc, elem, "sframe");
	m_endPointModel.saveSettings(doc, elem, "eframe");
	m_grainSizeModel.saveSettings(doc, elem, "grainsize");
	m_grainPositionModel.saveSettings(doc, elem, "grainpos");
	m_spreadModel.saveSettings(doc, elem, "spread");
	m_numGrainsModel.saveSettings(doc, elem, "numgrains");
	m_loopPointModel.saveSettings(doc, elem, "lframe");
	m_stutterModel.saveSettings(doc, elem, "stutter");
	m_interpolationModel.saveSettings(doc, elem, "interp");
}




void Granulator::loadSettings(const QDomElement& elem)
{
	if (auto srcFile = elem.attribute("src"); !srcFile.isEmpty())
	{
		if (QFileInfo(PathUtil::toAbsolute(srcFile)).exists())
		{
			setAudioFile(srcFile, false);
		}
		else { Engine::getSong()->collectError(QString("%1: %2").arg(tr("Sample not found"), srcFile)); }
	}
	else if (auto sampleData = elem.attribute("sampledata"); !sampleData.isEmpty())
	{
		m_sample = Sample(gui::SampleLoader::createBufferFromBase64(sampleData));
	}

	m_loopModel.loadSettings(elem, "looped");
	m_ampModel.loadSettings(elem, "amp");
	m_endPointModel.loadSettings(elem, "eframe");
	m_startPointModel.loadSettings(elem, "sframe");
	m_grainSizeModel.loadSettings(elem, "grainsize");
	m_grainPositionModel.loadSettings(elem, "grainpos");
	m_spreadModel.loadSettings(elem, "spread");
	m_numGrainsModel.loadSettings(elem, "numgrains");

	// compat code for not having a separate loopback point
	if (elem.hasAttribute("lframe") || !elem.firstChildElement("lframe").isNull())
	{
		m_loopPointModel.loadSettings(elem, "lframe");
	}
	else
	{
		m_loopPointModel.loadSettings(elem, "sframe");
	}

	m_reverseModel.loadSettings(elem, "reversed");

	m_stutterModel.loadSettings(elem, "stutter");
	if (elem.hasAttribute("interp") || !elem.firstChildElement("interp").isNull())
	{
		m_interpolationModel.loadSettings(elem, "interp");
	}
	else
	{
		m_interpolationModel.setValue(1.0f); // linear by default
	}

	pointChanged();
	emit sampleUpdated();
}




void Granulator::loadFile( const QString & _file )
{
	setAudioFile( _file );
}




QString Granulator::nodeName() const
{
	return granulator_plugin_descriptor.name;
}




auto Granulator::beatLen(NotePlayHandle* note) const -> int
{
	// If we can play indefinitely, use the default beat note duration
	if (static_cast<Sample::Loop>(m_loopModel.value()) != Sample::Loop::Off) { return 0; }

	// Otherwise, use the remaining sample duration
	const auto baseFreq = instrumentTrack()->baseFreq();
	const auto freqFactor = baseFreq / note->frequency()
		* Engine::audioEngine()->outputSampleRate()
		/ Engine::audioEngine()->baseSampleRate();

	const auto startFrame = m_nextPlayStartPoint >= m_sample.endFrame()
		? m_sample.startFrame()
		: m_nextPlayStartPoint;
	const auto duration = m_sample.endFrame() - startFrame;

	return static_cast<int>(std::floor(duration * freqFactor));
}




gui::PluginView* Granulator::instantiateView( QWidget * _parent )
{
	return new gui::GranulatorView( this, _parent );
}

void Granulator::setAudioFile(const QString& _audio_file, bool _rename)
{
	// is current channel-name equal to previous-filename??
	if( _rename &&
		( instrumentTrack()->name() ==
			QFileInfo(m_sample.sampleFile()).fileName() ||
				m_sample.sampleFile().isEmpty()))
	{
		// then set it to new one
		instrumentTrack()->setName( PathUtil::cleanName( _audio_file ) );
	}
	// else we don't touch the track-name, because the user named it self

	m_sample = Sample(gui::SampleLoader::createBufferFromFile(_audio_file));
	loopPointChanged();
	emit sampleUpdated();
}




void Granulator::reverseModelChanged()
{
	m_sample.setReversed(m_reverseModel.value());
	m_nextPlayStartPoint = m_sample.startFrame();
	m_nextPlayBackwards = false;
	emit sampleUpdated();
}




void Granulator::ampModelChanged()
{
	m_sample.setAmplification(m_ampModel.value() / 100.0f);
	emit sampleUpdated();
}


void Granulator::stutterModelChanged()
{
	m_nextPlayStartPoint = m_sample.startFrame();
	m_nextPlayBackwards = false;
}


void Granulator::startPointChanged()
{
	// check if start is over end and swap values if so
	if( m_startPointModel.value() > m_endPointModel.value() )
	{
		float tmp = m_endPointModel.value();
		m_endPointModel.setValue( m_startPointModel.value() );
		m_startPointModel.setValue( tmp );
	}

	// nudge loop point with end
	if( m_loopPointModel.value() >= m_endPointModel.value() )
	{
		m_loopPointModel.setValue( qMax( m_endPointModel.value() - 0.001f, 0.0f ) );
	}

	// nudge loop point with start
	if( m_loopPointModel.value() < m_startPointModel.value() )
	{
		m_loopPointModel.setValue( m_startPointModel.value() );
	}

	// check if start & end overlap and nudge end up if so
	if( m_startPointModel.value() == m_endPointModel.value() )
	{
		m_endPointModel.setValue( qMin( m_endPointModel.value() + 0.001f, 1.0f ) );
	}

	pointChanged();

}

void Granulator::endPointChanged()
{
	// same as start, for now
	startPointChanged();

}

void Granulator::grainSizeChanged()
{
}
void Granulator::grainPositionChanged()
{
}
void Granulator::spreadChanged()
{
}
void Granulator::numGrainsChanged()
{
}

void Granulator::loopPointChanged()
{

	// check that loop point is between start-end points and not overlapping with endpoint
	// ...and move start/end points ahead if loop point is moved over them
	if( m_loopPointModel.value() >= m_endPointModel.value() )
	{
		m_endPointModel.setValue( m_loopPointModel.value() + 0.001f );
		if( m_endPointModel.value() == 1.0f )
		{
			m_loopPointModel.setValue( 1.0f - 0.001f );
		}
	}

	// nudge start point with loop
	if( m_loopPointModel.value() < m_startPointModel.value() )
	{
		m_startPointModel.setValue( m_loopPointModel.value() );
	}

	pointChanged();
}

void Granulator::pointChanged()
{
	const auto f_start = static_cast<f_cnt_t>(m_startPointModel.value() * m_sample.sampleSize());
	const auto f_end = static_cast<f_cnt_t>(m_endPointModel.value() * m_sample.sampleSize());
	const auto f_loop = static_cast<f_cnt_t>(m_loopPointModel.value() * m_sample.sampleSize());

	m_nextPlayStartPoint = f_start;
	m_nextPlayBackwards = false;

	m_sample.setAllPointFrames(f_start, f_end, f_loop, f_end);
	emit dataChanged();
}


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model * model, void *)
{
	return new Granulator(static_cast<InstrumentTrack *>(model));
}


}


} // namespace lmms
