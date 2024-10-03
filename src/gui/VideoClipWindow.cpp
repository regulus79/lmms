/*
 * VideoClipWindow.cpp
 *
 * Copyright (c) 2024 regulus79
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


#include "GuiApplication.h"
#include "MainWindow.h"
#include "VideoClipWindow.h"
#include "SubWindow.h"

namespace lmms::gui
{

VideoClipWindow::VideoClipWindow(VideoClip * vclip):
	QWidget(),
    m_clip(vclip)
{
	QMdiSubWindow * subWin = getGUI()->mainWindow()->addWindowedWidget(this);
}

void VideoClipWindow::toggleVisibility(bool on)
{
	if(on)
	{
		show();
		parentWidget()->show();
		parentWidget()->raise();
	}
	else
	{
		parentWidget()->hide();
	}
}

} // namespace lmms::gui