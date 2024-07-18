/*
 * GranulatorView.cpp - originally AudioFileProcessorView.cpp, but edited by Regulus to be a granular synth.
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

#include "GranulatorView.h"

#include "Granulator.h"
#include "GranulatorWaveView.h"

#include <QPainter>

#include "ComboBox.h"
#include "DataFile.h"
#include "gui_templates.h"
#include "PixmapButton.h"
#include "SampleLoader.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "Track.h"
#include "Clipboard.h"


namespace lmms
{

namespace gui
{

GranulatorView::GranulatorView(Instrument* instrument,
							QWidget* parent) :
	InstrumentViewFixedSize(instrument, parent)
{
	m_openAudioFileButton = new PixmapButton(this);
	m_openAudioFileButton->setCursor(QCursor(Qt::PointingHandCursor));
	m_openAudioFileButton->move(227, 95);
	m_openAudioFileButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap(
							"select_file"));
	m_openAudioFileButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(
							"select_file"));
	connect(m_openAudioFileButton, SIGNAL(clicked()),
					this, SLOT(openAudioFile()));
	m_openAudioFileButton->setToolTip(tr("Open sample"));

	m_reverseButton = new PixmapButton(this);
	m_reverseButton->setCheckable(true);
	m_reverseButton->move(164, 105+25);
	m_reverseButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap(
							"reverse_on"));
	m_reverseButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(
							"reverse_off"));
	m_reverseButton->setToolTip(tr("Reverse sample"));

// loop button group

	auto m_loopOffButton = new PixmapButton(this);
	m_loopOffButton->setCheckable(true);
	m_loopOffButton->move(190, 105+25);
	m_loopOffButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap(
							"loop_off_on"));
	m_loopOffButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(
							"loop_off_off"));
	m_loopOffButton->setToolTip(tr("Disable loop"));

	auto m_loopOnButton = new PixmapButton(this);
	m_loopOnButton->setCheckable(true);
	m_loopOnButton->move(190, 124+25);
	m_loopOnButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap(
							"loop_on_on"));
	m_loopOnButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(
							"loop_on_off"));
	m_loopOnButton->setToolTip(tr("Enable loop"));

	auto m_loopPingPongButton = new PixmapButton(this);
	m_loopPingPongButton->setCheckable(true);
	m_loopPingPongButton->move(216, 124+25);
	m_loopPingPongButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap(
							"loop_pingpong_on"));
	m_loopPingPongButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(
							"loop_pingpong_off"));
	m_loopPingPongButton->setToolTip(tr("Enable ping-pong loop"));

	m_loopGroup = new automatableButtonGroup(this);
	m_loopGroup->addButton(m_loopOffButton);
	m_loopGroup->addButton(m_loopOnButton);
	m_loopGroup->addButton(m_loopPingPongButton);

	m_stutterButton = new PixmapButton(this);
	m_stutterButton->setCheckable(true);
	m_stutterButton->move(164, 124+25);
	m_stutterButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap(
								"stutter_on"));
	m_stutterButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(
								"stutter_off"));
	m_stutterButton->setToolTip(
		tr("Continue sample playback across notes"));

	m_ampKnob = new Knob(KnobType::Bright26, this);
	m_ampKnob->setVolumeKnob(true);
	m_ampKnob->move(10, 138-3);
	m_ampKnob->setHintText(tr("Amplify:"), "%");

	m_grainSizeKnob = new TempoSyncKnob(KnobType::Bright26, this);
	m_grainSizeKnob->move(10, 10-4);
	m_grainSizeKnob->setHintText(tr("Grain Size:"), "s");

	m_grainPositionKnob = new Knob(KnobType::Bright26, this);
	m_grainPositionKnob->move(45, 10-4);
	m_grainPositionKnob->setHintText(tr("Grain Position:"), "");

	m_spreadKnob = new Knob(KnobType::Bright26, this);
	m_spreadKnob->move(45, 45-4);
	m_spreadKnob->setHintText(tr("Spread:"), "");

	m_numGrainsKnob = new Knob(KnobType::Bright26, this);
	m_numGrainsKnob->move(10, 45-4);
	m_numGrainsKnob->setHintText(tr("Number of grains:"), "");

	m_scanRateKnob = new Knob(KnobType::Bright26, this);
	m_scanRateKnob->move(10, 80-4);
	m_scanRateKnob->setHintText(tr("Scan Rate:"), "");

	m_widthKnob = new Knob(KnobType::Bright26, this);
	m_widthKnob->move(45, 80-4);
	m_widthKnob->setHintText(tr("Width:"), "%");

	m_startKnob = new GranulatorWaveView::knob(this);
	m_startKnob->move(45, 138-3);
	m_startKnob->setHintText(tr("Start point:"), "");

	m_endKnob = new GranulatorWaveView::knob(this);
	m_endKnob->move(125, 138-3);
	m_endKnob->setHintText(tr("End point:"), "");

	m_loopKnob = new GranulatorWaveView::knob(this);
	m_loopKnob->move(85, 138-3);
	m_loopKnob->setHintText(tr("Loopback point:"), "");

// interpolation selector
	m_interpBox = new ComboBox(this);
	m_interpBox->setGeometry(142, 82+5, 82, ComboBox::DEFAULT_HEIGHT);

// wavegraph
	m_waveView = 0;
	newWaveView();

	connect(castModel<Granulator>(), SIGNAL(isPlaying(lmms::f_cnt_t)),
			m_waveView, SLOT(isPlaying(lmms::f_cnt_t)));

	qRegisterMetaType<lmms::f_cnt_t>("lmms::f_cnt_t");

	setAcceptDrops(true);
}

void GranulatorView::dragEnterEvent(QDragEnterEvent* dee)
{
	// For mimeType() and MimeType enum class
	using namespace Clipboard;

	if (dee->mimeData()->hasFormat(mimeType(MimeType::StringPair)))
	{
		QString txt = dee->mimeData()->data(
						mimeType(MimeType::StringPair));
		if (txt.section(':', 0, 0) == QString("clip_%1").arg(
							static_cast<int>(Track::Type::Sample)))
		{
			dee->acceptProposedAction();
		}
		else if (txt.section(':', 0, 0) == "samplefile")
		{
			dee->acceptProposedAction();
		}
		else
		{
			dee->ignore();
		}
	}
	else
	{
		dee->ignore();
	}
}

void GranulatorView::newWaveView()
{
	if (m_waveView)
	{
		delete m_waveView;
		m_waveView = 0;
	}
	m_waveView = new GranulatorWaveView(this, 245, 75, &castModel<Granulator>()->sample(),
		dynamic_cast<GranulatorWaveView::knob*>(m_startKnob),
		dynamic_cast<GranulatorWaveView::knob*>(m_endKnob),
		dynamic_cast<GranulatorWaveView::knob*>(m_loopKnob));
	m_waveView->move(2, 172);
	
	m_waveView->show();
}

void GranulatorView::dropEvent(QDropEvent* de)
{
	const auto type = StringPairDrag::decodeKey(de);
	const auto value = StringPairDrag::decodeValue(de);

	if (type == "samplefile") { castModel<Granulator>()->setAudioFile(value); }
	else if (type == QString("clip_%1").arg(static_cast<int>(Track::Type::Sample)))
	{
		DataFile dataFile(value.toUtf8());
		castModel<Granulator>()->setAudioFile(dataFile.content().firstChild().toElement().attribute("src"));
	}
	else
	{
		de->ignore();
		return;
	}

	m_waveView->updateSampleRange();
	Engine::getSong()->setModified();
	de->accept();
}

void GranulatorView::paintEvent(QPaintEvent*)
{
	QPainter p(this);

	static auto s_artwork = PLUGIN_NAME::getIconPixmap("artwork");
	p.drawPixmap(0, 0, s_artwork);

	auto a = castModel<Granulator>();

	QString file_name = "";

	int idx = a->sample().sampleFile().length();

	p.setFont(adjustedToPixelSize(font(), 8));

	QFontMetrics fm(p.font());

	// simple algorithm for creating a text from the filename that
	// matches in the white rectangle
	while(idx > 0 &&
		fm.size(Qt::TextSingleLine, file_name + "...").width() < 210)
	{
		file_name = a->sample().sampleFile()[--idx] + file_name;
	}

	if (idx > 0)
	{
		file_name = "..." + file_name;
	}

	p.setPen(QColor(255, 255, 255));
	p.drawText(8, 122, file_name);
}

void GranulatorView::sampleUpdated()
{
	m_waveView->updateSampleRange();
	m_waveView->update();
	update();
}

void GranulatorView::openAudioFile()
{
	QString af = SampleLoader::openAudioFile();
	if (af.isEmpty()) { return; }

	castModel<Granulator>()->setAudioFile(af);
	Engine::getSong()->setModified();
	m_waveView->updateSampleRange();
}

void GranulatorView::modelChanged()
{
	auto a = castModel<Granulator>();
	connect(a, &Granulator::sampleUpdated, this, &GranulatorView::sampleUpdated);
	m_ampKnob->setModel(&a->ampModel());
	m_grainSizeKnob->setModel(&a->grainSizeModel());
	m_grainPositionKnob->setModel(&a->grainPositionModel());
	m_spreadKnob->setModel(&a->spreadModel());
	m_numGrainsKnob->setModel(&a->numGrainsModel());
	m_scanRateKnob->setModel(&a->scanRateModel());
	m_widthKnob->setModel(&a->widthModel());
	m_startKnob->setModel(&a->startPointModel());
	m_endKnob->setModel(&a->endPointModel());
	m_loopKnob->setModel(&a->loopPointModel());
	m_reverseButton->setModel(&a->reverseModel());
	m_loopGroup->setModel(&a->loopModel());
	m_stutterButton->setModel(&a->stutterModel());
	m_interpBox->setModel(&a->interpolationModel());
	sampleUpdated();
}

} // namespace gui

} // namespace lmms
