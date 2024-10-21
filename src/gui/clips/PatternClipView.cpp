/*
 * PatternClipView.cpp
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

#include "PatternClipView.h"

#include <QApplication>
#include <QMenu>
#include <QPainter>

#include "AutomationClip.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "MidiClip.h"
#include "PatternClip.h"
#include "PatternStore.h"
#include "PatternTrack.h"
#include "RenameDialog.h"
#include "SampleClip.h"

namespace lmms::gui
{


PatternClipView::PatternClipView(Clip* _clip, TrackView* _tv) :
	ClipView( _clip, _tv ),
	m_patternClip(dynamic_cast<PatternClip*>(_clip)),
	m_paintPixmap()
{
	connect( _clip->getTrack(), SIGNAL(dataChanged()), 
			this, SLOT(update()));

	setStyle( QApplication::style() );
}

void PatternClipView::constructContextMenu(QMenu* _cm)
{
	auto a = new QAction(embed::getIconPixmap("pattern_track"), tr("Open in Pattern Editor"), _cm);
	_cm->insertAction( _cm->actions()[0], a );
	connect( a, SIGNAL(triggered(bool)),
			this, SLOT(openInPatternEditor()));
	_cm->insertSeparator( _cm->actions()[1] );
	_cm->addSeparator();
	_cm->addAction( embed::getIconPixmap( "reload" ), tr( "Reset name" ),
						this, SLOT(resetName()));
	_cm->addAction( embed::getIconPixmap( "edit_rename" ),
						tr( "Change name" ),
						this, SLOT(changeName()));
}




void PatternClipView::mouseDoubleClickEvent(QMouseEvent*)
{
	openInPatternEditor();
}




void PatternClipView::paintEvent(QPaintEvent*)
{
	QPainter painter( this );

	if( !needsUpdate() )
	{
		painter.drawPixmap( 0, 0, m_paintPixmap );
		return;
	}

	setNeedsUpdate( false );

	if (m_paintPixmap.isNull() || m_paintPixmap.size() != size())
	{
		m_paintPixmap = QPixmap(size());
	}

	QPainter p( &m_paintPixmap );

	QLinearGradient lingrad( 0, 0, 0, height() );
	QColor c = getColorForDisplay( painter.background().color() );
	
	lingrad.setColorAt( 0, c.lighter( 130 ) );
	lingrad.setColorAt( 1, c.lighter( 70 ) );

	// paint a black rectangle under the clip to prevent glitches with transparent backgrounds
	p.fillRect( rect(), QColor( 0, 0, 0 ) );

	int pixelsPerPattern = Engine::patternStore()->lengthOfPattern(m_patternClip->patternIndex()) * pixelsPerBar();
	int offset = static_cast<int>(m_patternClip->startTimeOffset() * (pixelsPerBar() / TimePos::ticksPerBar()))
			% pixelsPerPattern;
	if (offset < 2) {
		offset += pixelsPerPattern;
	}

	if( gradient() )
	{
		p.fillRect( rect(), lingrad );
	}
	else
	{
		p.fillRect( rect(), c );
	}
	// draw notes
	int patternIndex = static_cast<PatternTrack*>(m_patternClip->getTrack())->patternIndex();
	// Count the number of non-empty instrument tracks. Only used midi tracks will be drawn.
	int numberIntsrumentTracksUsed = 0;
	for (const auto& track : Engine::patternStore()->tracks())
	{
		Clip* clip = track->getClip(patternIndex);
		MidiClip* mClip = dynamic_cast<MidiClip*>(clip);
		if (mClip)
		{
			if (mClip->notes().size() > 0) { numberIntsrumentTracksUsed++; }
		}
	}

	int trackIndex = 0;
	for (const auto& track : Engine::patternStore()->tracks())
	{
		Clip* clip = track->getClip(patternIndex);
		MidiClip* mClip = dynamic_cast<MidiClip*>(clip);
		SampleClip* sClip = dynamic_cast<SampleClip*>(clip);
		AutomationClip* aClip = dynamic_cast<AutomationClip*>(clip);
		if (mClip)
		{
			if (mClip->notes().size() == 0) { continue; } // Continue without updating the track index; empty tracks are not drawn.
			// Compare how long the clip view is compared to the underlying pattern. First +1 for ceiling, second +1 for possible previous bar.
			int maxPossibleRepeitions = getClip()->length() / mClip->length() + 1 + 1;
			for (Note const * note : mClip->notes())
			{
				QRect noteRect = QRect(
					note->pos() * pixelsPerBar() / TimePos::ticksPerBar() + offset,
					trackIndex * height() / numberIntsrumentTracksUsed,
					pixelsPerBar() / 16,
					height() / numberIntsrumentTracksUsed
				);
				// Loop through all the possible bars this pattern could affect. Starting at -1 for the possibility of start offset
				for (int i = -1; i < maxPossibleRepeitions - 1; i++)
				{
					noteRect.moveLeft(note->pos() * pixelsPerBar() / TimePos::ticksPerBar() + offset + i * pixelsPerPattern);
					p.fillRect(noteRect, QColor(255, 255, 255, std::min(255, note->getVolume() * 255 / 100)));
				}
			}
		}
		else if (sClip)
		{
			// TODO, we probably won't be rendering a whole waveform for this; perhaps just a single block given the sample start offset
		}
		else if (aClip)
		{
			// TODO, probably do nothing?
		}
		// Only increment the track index if we actually drew anything; empty tracks do not contribute.
		trackIndex++;
	}
	
	// bar lines
	const int lineSize = 3;

	p.setPen( c.darker( 200 ) );

	if (pixelsPerPattern > 0)
	{
		for (int x = offset; x < width() - 2; x += pixelsPerPattern)
		{
			p.drawLine( x, BORDER_WIDTH, x, BORDER_WIDTH + lineSize );
			p.drawLine( x, rect().bottom() - ( BORDER_WIDTH + lineSize ),
			 	x, rect().bottom() - BORDER_WIDTH );
		}
	}

	// clip name
	paintTextLabel(m_patternClip->name(), p);

	// inner border
	p.setPen( c.lighter( 130 ) );
	p.drawRect( 1, 1, rect().right() - BORDER_WIDTH,
		rect().bottom() - BORDER_WIDTH );	

	// outer border
	p.setPen( c.darker( 300 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );
	
	// draw the 'muted' pixmap only if the clip was manualy muted
	if (m_patternClip->isMuted())
	{
		const int spacing = BORDER_WIDTH;
		const int size = 14;
		p.drawPixmap( spacing, height() - ( size + spacing ),
			embed::getIconPixmap( "muted", size, size ) );
	}
	
	p.end();
	
	painter.drawPixmap( 0, 0, m_paintPixmap );
}




void PatternClipView::openInPatternEditor()
{
	Engine::patternStore()->setCurrentPattern(m_patternClip->patternIndex());

	getGUI()->mainWindow()->togglePatternEditorWin(true);
}




void PatternClipView::resetName() { m_patternClip->setName(""); }




void PatternClipView::changeName()
{
	QString s = m_patternClip->name();
	RenameDialog rename_dlg( s );
	rename_dlg.exec();
	m_patternClip->setName(s);
}



void PatternClipView::update()
{
	setToolTip(m_patternClip->name());

	ClipView::update();
}


} // namespace lmms::gui
