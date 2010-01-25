/*
 * peak_controller_effect.cpp - PeakController effect plugin
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#include "Controller.h"
#include "song.h"
#include "PeakController.h"
#include "peak_controller_effect.h"

#include "embed.cpp"


extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT peakcontrollereffect_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Peak Controller",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Plugin for controlling knobs with sound peaks" ),
	"Paul Giblock <drfaygo/at/gmail.com>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}

// We have to keep a list of all the PeakController effects so that we can save
// an peakEffect-ID to the project.  This ID is referenced in the PeakController
// settings and is used to set the PeakControllerEffect pointer upon load

//QVector<PeakControllerEffect *> PeakControllerEffect::s_effects;

PeakControllerEffect::PeakControllerEffect(
			Model * _parent,
			const Descriptor::SubPluginFeatures::Key * _key ) :
	Effect( &peakcontrollereffect_plugin_descriptor, _parent, _key ),
	m_peakControls( this ),
	m_autoController( NULL )
{
	m_autoController = new PeakController( engine::getSong(), this );
	engine::getSong()->addController( m_autoController );
	PeakController::s_effects.append( this );
}




PeakControllerEffect::~PeakControllerEffect()
{
	int idx = PeakController::s_effects.indexOf( this );
	if( idx >= 0 )
	{
		PeakController::s_effects.remove( idx );
	}
}



bool PeakControllerEffect::processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames )
{
	PeakControllerEffectControls & c = m_peakControls;
	
	// This appears to be used for determining whether or not to continue processing
	// audio with this effect	
	if( !isEnabled() || !isRunning() )
	{
		return( FALSE );
	}


	// RMS:
	double sum = 0;
	for( int i = 0; i < _frames; ++i )
	{
		sum += _buf[i][0]*_buf[i][0] + _buf[i][1]*_buf[i][1];
	}

	if( c.m_muteModel.value() )
	{
		for( int i = 0; i < _frames; ++i )
		{
			_buf[i][0] = _buf[i][1] = 0.0f;
		}
	}

	m_lastSample = c.m_baseModel.value() + c.m_amountModel.value() *
							sqrtf( sum / _frames );

	//checkGate( out_sum / _frames );

	return( isRunning() );
}




extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model * _parent, void * _data )
{
	return new PeakControllerEffect( _parent,
		static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(
								_data ) );
}

}

