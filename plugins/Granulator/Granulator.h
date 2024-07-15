/*
 * Granulator.h - declaration of class Granulator
 *                          originally AudioFileProcessor, but edited by Regulus to be a granular synth.
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

#ifndef LMMS_GRANULATOR_H
#define LMMS_GRANULATOR_H


#include "AutomatableModel.h"
#include "ComboBoxModel.h"
#include "TempoSyncKnobModel.h"

#include "Instrument.h"
#include "Sample.h"


namespace lmms
{

class Granulator : public Instrument
{
	Q_OBJECT
public:
	Granulator( InstrumentTrack * _instrument_track );

	int getNewGrainStartFrame();
	bool addGrain( NotePlayHandle * _n, SampleFrame* _working_buffer, int grain_index, int grain_size, f_cnt_t grain_offset );

	void playNote( NotePlayHandle * _n,
						SampleFrame* _working_buffer ) override;
	void deleteNotePluginData( NotePlayHandle * _n ) override;

	void saveSettings(QDomDocument& doc, QDomElement& elem) override;
	void loadSettings(const QDomElement& elem) override;

	void loadFile( const QString & _file ) override;

	QString nodeName() const override;

	auto beatLen(NotePlayHandle* note) const -> int override;

	float desiredReleaseTimeMs() const override
	{
		return 3.f;
	}

	gui::PluginView* instantiateView( QWidget * _parent ) override;

	Sample const & sample() const { return m_sample; }

	FloatModel & ampModel() { return m_ampModel; }
	TempoSyncKnobModel & grainSizeModel() { return m_grainSizeModel; }
	FloatModel & grainPositionModel() { return m_grainPositionModel; }
	FloatModel & spreadModel() { return m_spreadModel; }
	FloatModel & numGrainsModel() { return m_numGrainsModel; }
	FloatModel & startPointModel() { return m_startPointModel; }
	FloatModel & endPointModel() { return m_endPointModel; }
	FloatModel & loopPointModel() { return m_loopPointModel; }
	BoolModel & reverseModel() { return m_reverseModel; }
	IntModel & loopModel() { return m_loopModel; }
	BoolModel & stutterModel() { return m_stutterModel; }
	ComboBoxModel & interpolationModel() { return m_interpolationModel; }


public slots:
	void setAudioFile(const QString& _audio_file, bool _rename = true);

private slots:
	void reverseModelChanged();
	void ampModelChanged();
	void grainSizeChanged();
	void grainPositionChanged();
	void spreadChanged();
	void numGrainsChanged();
	void loopPointChanged();
	void startPointChanged();
	void endPointChanged();
	void pointChanged();
	void stutterModelChanged();


signals:
	void isPlaying( lmms::f_cnt_t _current_frame );
	void sampleUpdated();

private:
	Sample m_sample;

	FloatModel m_ampModel;
	TempoSyncKnobModel m_grainSizeModel;
	FloatModel m_grainPositionModel;
	FloatModel m_spreadModel;
	FloatModel m_numGrainsModel;
	FloatModel m_startPointModel;
	FloatModel m_endPointModel;
	FloatModel m_loopPointModel;
	BoolModel m_reverseModel;
	IntModel m_loopModel;
	BoolModel m_stutterModel;
	ComboBoxModel m_interpolationModel;

	f_cnt_t m_nextPlayStartPoint;
	bool m_nextPlayBackwards;
} ;

} // namespace lmms

#endif // LMMS_GRANULATOR_H
