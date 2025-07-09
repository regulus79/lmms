/*
 * TrackView.h - declaration of TrackView class
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

#ifndef LMMS_GUI_TRACK_VIEW_H
#define LMMS_GUI_TRACK_VIEW_H

#include <QWidget>

#include "JournallingObject.h"
#include "ModelView.h"
#include "TrackContentWidget.h"
#include "TrackOperationsWidget.h"

class QMenu;

namespace lmms
{

class Track;
class Clip;


namespace gui
{

class FadeButton;
class TrackContainerView;
class TrackResizeLine;


const int TRACK_OP_WIDTH = 78;


class TrackView : public QWidget, public ModelView, public JournallingObject
{
	Q_OBJECT
public:
	/*! The width of the resize grip in pixels */
	static constexpr int RESIZE_GRIP_WIDTH = 4;

	TrackView( Track * _track, TrackContainerView* tcv );
	~TrackView() override = default;

	inline const Track * getTrack() const
	{
		return m_track;
	}

	inline Track * getTrack()
	{
		return m_track;
	}

	inline TrackContainerView* trackContainerView()
	{
		return m_trackContainerView;
	}

	inline TrackOperationsWidget * getTrackOperationsWidget()
	{
		return &m_trackOperationsWidget;
	}

	inline QWidget * getTrackSettingsWidget()
	{
		return &m_trackSettingsWidget;
	}

	inline TrackContentWidget * getTrackContentWidget()
	{
		return &m_trackContentWidget;
	}

	bool isMovingTrack() const
	{
		return m_action == Action::Move;
	}

	virtual void update();

	// Create a menu for assigning/creating channels for this track
	// Currently instrument track and sample track supports it
	virtual QMenu * createMixerMenu(QString title, QString newMixerLabel);


public slots:
	virtual bool close();


protected:
	void modelChanged() override;

	void saveSettings( QDomDocument& doc, QDomElement& element ) override
	{
		Q_UNUSED(doc)
		Q_UNUSED(element)
	}

	void loadSettings( const QDomElement& element ) override
	{
		Q_UNUSED(element)
	}

	QString nodeName() const override
	{
		return "trackview";
	}


	void dragEnterEvent( QDragEnterEvent * dee ) override;
	void dropEvent( QDropEvent * de ) override;
	void mouseMoveEvent( QMouseEvent * me ) override;
	void wheelEvent(QWheelEvent* we) override;
	void paintEvent( QPaintEvent * pe ) override;
	void resizeEvent( QResizeEvent * re ) override;

private:
	void resizeToHeight(int height);

private:
	enum class Action
	{
		None,
		Move,
		ResizeVertical,
		ResizeHorizontal
	} ;

	Track * m_track;
	TrackContainerView * m_trackContainerView;

	// Widget that contains the menu, mute and solo buttons
	TrackOperationsWidget m_trackOperationsWidget;
	// Empty widget where each track class may add a label, FX channel, volume knobs, etc
	QWidget m_trackSettingsWidget;
	// Widget that is the timeline where ClipViews are placed
	TrackContentWidget m_trackContentWidget;
	// Resize handle at the bottom of each track
	TrackResizeLine* m_heightResizeLine;
	// Resize handle to the right of track label and controls
	TrackResizeLine* m_widthResizeLine;

	Action m_action;

	virtual FadeButton * getActivityIndicator()
	{
		return nullptr;
	}

	void setIndicatorMute(FadeButton* indicator, bool muted);

	friend class TrackLabelButton;
	friend class TrackResizeLine;


private slots:
	void createClipView( lmms::Clip * clip );
	void muteChanged();
	void onTrackGripGrabbed();
	void onTrackGripReleased();
	void updateTrackHeadWidth(int width);
} ;


class TrackResizeLine : public QWidget
{
	Q_OBJECT
public:
	TrackResizeLine(TrackView* tv, TrackView::Action action, Qt::CursorShape cursor);
protected:
	void mousePressEvent(QMouseEvent* me) override;
	void mouseReleaseEvent(QMouseEvent* me) override;
private:
	TrackView* m_trackView;
	TrackView::Action m_action;
};


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_TRACK_VIEW_H
