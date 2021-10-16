///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2021 Mihai Ursu                                                 //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

/*
SurfBeam2.cpp

This file contains the sources for the ViaSat SurfBeam 2 modem.
*/

#include "SurfBeam2.h"
#include "ui_SurfBeam2.h"

#include <QTimer>
#include <QtNetwork>

#include <fstream>
#include <iostream>

#include <math.h>
#include <string.h>


//!************************************************************************
//! Constructor
//!************************************************************************
SurfBeam2::SurfBeam2
    (
    QWidget*    aParent     //!< a parent widget
    )
    : QMainWindow( aParent )
    , mMainUi( new Ui::SurfBeam2 )
{
    mMainUi->setupUi( this );

    //****************************************
    // progress bar setup
    //****************************************
    mMainUi->rxSnrProgressbar->setStyleSheet(
    "QProgressBar { background-color: rgb( 191, 191, 191 ); border-radius: 5px; } QProgressBar::chunk { background-color: rgb( 0, 191, 0 ); border-radius: 5px; }" );

    mMainUi->rxRfPowerProgressbar->setStyleSheet(
    "QProgressBar { background-color: rgb( 191, 191, 191 ); border-radius: 5px; } QProgressBar::chunk { background-color: rgb( 0, 191, 0 ); border-radius: 5px; }" );


    mMainUi->txIfPowerProgressbar->setStyleSheet(
    "QProgressBar { background-color: rgb( 191, 191, 191 ); border-radius: 5px; } QProgressBar::chunk { background-color: rgb( 0, 191, 0 ); border-radius: 5px; }" );

    mMainUi->txRfPowerProgressbar->setStyleSheet(
    "QProgressBar { background-color: rgb( 191, 191, 191 ); border-radius: 5px; } QProgressBar::chunk { background-color: rgb( 0, 191, 0 ); border-radius: 5px; }" );


    mMainUi->cableAttenuationProgressbar->setStyleSheet(
    "QProgressBar { background-color: rgb( 191, 191, 191 ); border-radius: 5px; } QProgressBar::chunk { background-color: rgb( 0, 191, 0 ); border-radius: 5px; }" );

    mMainUi->cableResistanceProgressbar->setStyleSheet(
    "QProgressBar { background-color: rgb( 191, 191, 191 ); border-radius: 5px; } QProgressBar::chunk { background-color: rgb( 0, 191, 0 ); border-radius: 5px; }" );

    //****************************************
    // timer
    //****************************************
    QTimer* cgiRequestTimer = new QTimer( this );
    connect( cgiRequestTimer, SIGNAL( timeout() ), this, SLOT( startCgiRequest() ) );
    const uint32_t CGI_REQUEST_MS = 500;
    cgiRequestTimer->start( CGI_REQUEST_MS );
    startCgiRequest();
} 

//!************************************************************************
//! Destructor
//!************************************************************************
SurfBeam2::~SurfBeam2()
{
    delete mMainUi;
} 

//!************************************************************************
//! Convert power from dBm to Watts
//!
//! @returns: power in Watts
//!************************************************************************
double SurfBeam2::convertDbmToWatts
    (
    const double aDbm   //!< power in dBm
    )
{
    return pow( 10.0, 0.1 * ( aDbm - 30.0 ) );
}

//!************************************************************************
//! Convert a power in dBm to a string in Watts, with a submultiple or a
//! multiple suffix.
//!
//! @returns: a string with the power conversion
//!************************************************************************
QString SurfBeam2::convertDbmToQstring
    (
    const double aDbm   //!< power in dBm
    )
{
    QString pwrString;
    double pwrWatts = convertDbmToWatts( aDbm );

    if( fabs( pwrWatts ) >= 1.0 )
    {
        pwrString = QString::number( pwrWatts, 'f', 3 ) + " W";
    }
    else if( fabs( pwrWatts ) >= 1.0e-3 )
    {
        pwrString = QString::number( pwrWatts * 1.e3, 'f', 1 ) + " mW";
    }
    else if( fabs( pwrWatts ) >= 1.0e-6 )
    {
        pwrString = QString::number( pwrWatts * 1.e6, 'f', 1 ) + " " + MU_SMALL + "W";
    }
    else if( fabs( pwrWatts ) >= 1.0e-9 )
    {
        pwrString = QString::number( pwrWatts * 1.e9, 'f', 1 ) + " nW";
    }
    else if( fabs( pwrWatts ) >= 1.0e-12 )
    {
        pwrString = QString::number( pwrWatts * 1.e12, 'f', 1 ) + " pW";
    }
    else if( fabs( pwrWatts ) >= 1.0e-15 )
    {
        pwrString = QString::number( pwrWatts * 1.e15, 'f', 1 ) + " fW";
    }
    else
    {
        pwrString = QString::number( pwrWatts, 'g', 3 ) + " W";
    }

    return pwrString;
}

//!************************************************************************
//! Convert a cable attenuation in dB to a percent, using a first degree polynomial interpolation.
//!
//! @returns: the cable attenuation in percent
//!************************************************************************
double SurfBeam2::getCableAttenuationPercent
    (
    const double aCableAttenuationDb    //!< attenuation in dB
    )
{
    double percent = 0;

    if( aCableAttenuationDb >= 0.0 )
    {
        percent = aCableAttenuationDb * 6.66666;
    }

    return percent;
} 

//!************************************************************************
//! Convert a Rx power in dBm to a percent, using a first degree polynomial interpolation.
//!
//! @returns: the Rx power in percent
//!************************************************************************
double SurfBeam2::getRxPwrPercent
    (
    const double aRxPwrDbm  //!< power in dBm
    )
{
    double percent = 0;

    if( aRxPwrDbm >= -72.586 )
    {
        percent = 119.42208 + aRxPwrDbm * 1.64524;
    }

    return percent;
} 

//!************************************************************************
//! Convert a Rx SNR in dB to a percent, using a first degree polynomial interpolation.
//!
//! @returns: the Rx SNR in percent
//!************************************************************************
double SurfBeam2::getRxSnrPercent
    (
    const double aRxSnrDb   //!< SNR in dB
    )
{
    double percent = 0;

    if( aRxSnrDb >= -3.0 )
    {
        percent = 10.71429 + aRxSnrDb * 3.57143;
    }

    return percent;
}

//!************************************************************************
//! Convert a Tx IF power in dBm to a percent, using a first degree polynomial interpolation.
//!
//! @returns: the Tx IF power in percent
//!************************************************************************
double SurfBeam2::getTxIfPwrPercent
    (
    const double aTxIfPwrDbm    //!< power in dBm
    )
{
    double percent = 0;

    if( aTxIfPwrDbm >= -35.5 )
    {
        percent = 137.86408 + aTxIfPwrDbm * 3.8835;
    }

    return percent;
} 

//!************************************************************************
//! Convert a Tx RF power in dBm to a percent, using a first degree polynomial interpolation.
//!
//! @returns: the Tx RF power in percent
//!************************************************************************
double SurfBeam2::getTxRfPwrPercent
    (
    const double aTxRfPwrDbm    //!< power in dBm
    )
{
    double percent = 0;

    if( aTxRfPwrDbm >= 14.5 )
    {
        percent = -56.31068 + aTxRfPwrDbm * 3.8835;
    }

    return percent;
} 

//!************************************************************************
//! Slot connected to the modem network reply finished signal.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void SurfBeam2::httpFinishedModem()
{
    QString rawString = QString::fromStdString( mByteArrayModem.toStdString() );
    mModemRawStringsList = rawString.split( FIELD_DELIMITER );

    // Important: the left-hand term needs to be checked after each firmware update
    if( FIELD_COUNT_MODEM == mModemRawStringsList.size() )
    {
        updateModemInfo();
        updateContent();
    }

    if( mReplyModem->error() )
    {
    }

    mReplyModem->deleteLater();
}

//!************************************************************************
//! Slot connected to the TRIA network reply finished signal.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void SurfBeam2::httpFinishedTria()
{
    QString rawString = QString::fromStdString( mByteArrayTria.toStdString() );
    mTriaRawStringsList = rawString.split( FIELD_DELIMITER );

    // Important: the left-hand term needs to be checked after each firmware update
    if( FIELD_COUNT_TRIA == mTriaRawStringsList.size() )
    {
        updateTriaInfo();
        updateContent();
    }

    if( mReplyTria->error() )
    {
    }

    mReplyTria->deleteLater();
}

//!************************************************************************
//! Slot connected to the modem IO Device ready read signal.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void SurfBeam2::httpReadyReadModem()
{
    mByteArrayModem += mReplyModem->readAll();
}

//!************************************************************************
//! Slot connected to the TRIA IO Device ready read signal.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void SurfBeam2::httpReadyReadTria()
{
    mByteArrayTria += mReplyTria->readAll();
}

//!************************************************************************
//! Start the CGI requests from modem & TRIA predefined URLs.
//!
//! @returns: nothing
//!************************************************************************
/* slot */ void SurfBeam2::startCgiRequest()
{
    mByteArrayModem.clear();
    mReplyModem = mQnam.get( QNetworkRequest( URL_MODEM ) );
    connect( mReplyModem, &QNetworkReply::finished, this, &SurfBeam2::httpFinishedModem );
    connect( mReplyModem, &QIODevice::readyRead, this, &SurfBeam2::httpReadyReadModem );

    mByteArrayTria.clear();
    mReplyTria = mQnam.get( QNetworkRequest( URL_TRIA ) );
    connect( mReplyTria, &QNetworkReply::finished, this, &SurfBeam2::httpFinishedTria );
    connect( mReplyTria, &QIODevice::readyRead, this, &SurfBeam2::httpReadyReadTria );
}

//!************************************************************************
//! Update the content related to UI items.
//!
//! @returns: nothing
//!************************************************************************
void SurfBeam2::updateContent()
{
    //***************************************************************************
    // Modem State
    //***************************************************************************
    mMainUi->modemStateLabel->setText( mModemInfo.ModemStatusLabel );
    mMainUi->onlineTimeLabel->setText( mModemInfo.OnlineTime );
    mMainUi->ipAddressLabel->setText( mModemInfo.IpAddress );
    mMainUi->oduTelemetryLabel->setText( mModemInfo.OutdoorUnitTelemetryStatus );

    QString colorString;

    switch( mModemInfo.SatStatusBeamColor )
    {
        case SATELLITE_STATUS_BEAM_COLOR_BLUE:
            colorString = "Blue";
            mMainUi->modemColorLabel->setStyleSheet( "QLabel { color : blue; }" );
            break;

        case SATELLITE_STATUS_BEAM_COLOR_ORANGE:
            colorString = "Orange";
            mMainUi->modemColorLabel->setStyleSheet( "QLabel { color : orange; }" );
            break;

        case SATELLITE_STATUS_BEAM_COLOR_PURPLE:
            colorString = "Purple";
            mMainUi->modemColorLabel->setStyleSheet( "QLabel { color : purple; }" );
            break;

        case SATELLITE_STATUS_BEAM_COLOR_GREEN:
            colorString = "Green";
            mMainUi->modemColorLabel->setStyleSheet( "QLabel { color : green; }" );
            break;

        default:
            colorString = "unknown";
            mMainUi->modemColorLabel->setStyleSheet( "QLabel { color : black; }" );
            break;
    }

    mMainUi->modemColorLabel->setText( colorString );

    //***************************************************************************
    // Modem Properties
    //***************************************************************************
    mMainUi->modemSerialNumberLabel->setText( mModemInfo.SerialNumber );
    mMainUi->partNumberLabel->setText( mModemInfo.PartNr );
    mMainUi->hardwareVersionLabel->setText( mModemInfo.HwVersion );
    mMainUi->softwareVersionLabel->setText( mModemInfo.SwVersion );
    mMainUi->macAddressLabel->setText( mModemInfo.MacAddress );

    QString uplinkSR;
    
    if( mModemInfo.UplinkSymbolRate >= 1e6 )
    {
        uplinkSR = QString::number( mModemInfo.UplinkSymbolRate / 1.e6, 'f', 3 ) + " MSym/s";
    }
    else if( mModemInfo.UplinkSymbolRate >= 1e3 )
    {
        uplinkSR = QString::number( mModemInfo.UplinkSymbolRate / 1.e3, 'f', 3 ) + " kSym/s";
    }
    else
    {
        uplinkSR = QString::number( mModemInfo.UplinkSymbolRate ) + " Sym/s";
    }
    
    mMainUi->symbolRateFwdLabel->setText( uplinkSR );
    QString downlinkSR;
    
    if( mModemInfo.UplinkSymbolRate >= 1e6 )
    {
        downlinkSR = QString::number( mModemInfo.DownlinkSymbolRate / 1.e6, 'f', 3 ) + " MSym/s";
    }
    else if( mModemInfo.UplinkSymbolRate >= 1e3 )
    {
        downlinkSR = QString::number( mModemInfo.DownlinkSymbolRate / 1.e3, 'f', 3 ) + " kSym/s";
    }
    else
    {
        downlinkSR = QString::number( mModemInfo.DownlinkSymbolRate ) + " Sym/s";
    }
    
    mMainUi->symbolRateReturnLabel->setText( downlinkSR );

    mMainUi->modulationLabel->setText( mModemInfo.DownlinkModulation );

    //***************************************************************************
    // TRIA Properties
    //***************************************************************************
    mMainUi->triaSerialNumberLabel->setText( mTriaInfo.SerialNumber );
    mMainUi->firmwareVersionLabel->setText( mTriaInfo.FwVersion );
    mMainUi->temperatureLabel->setText( QString::number( mTriaInfo.TemperatureCelsius ) + " °C"  );

    QString polString = "unknown";
    
    if( mTriaInfo.PolarizationType.contains( "left", Qt::CaseInsensitive ) )
    {
         polString = "Circular Left";
    }
    else if( mTriaInfo.PolarizationType.contains( "right", Qt::CaseInsensitive ) )
    {
         polString = "Circular Right";
    }
    else if( mTriaInfo.PolarizationType.contains( "horiz", Qt::CaseInsensitive ) )
    {
         polString = "Horizontal";
    }
    else if( mTriaInfo.PolarizationType.contains( "vert", Qt::CaseInsensitive ) )
    {
         polString = "Vertical";
    }

    mMainUi->polarizationLabel->setText( polString );

    switch( mTriaInfo.SatStatusBeamColor )
    {
        case SATELLITE_STATUS_BEAM_COLOR_BLUE:
            colorString = "Blue";
            mMainUi->triaColorLabel->setStyleSheet( "QLabel { color : blue; }" );
            break;

        case SATELLITE_STATUS_BEAM_COLOR_ORANGE:
            colorString = "Orange";
            mMainUi->triaColorLabel->setStyleSheet( "QLabel { color : orange; }" );
            break;

        case SATELLITE_STATUS_BEAM_COLOR_PURPLE:
            colorString = "Purple";
            mMainUi->triaColorLabel->setStyleSheet( "QLabel { color : purple; }" );
            break;

        case SATELLITE_STATUS_BEAM_COLOR_GREEN:
            colorString = "Green";
            mMainUi->triaColorLabel->setStyleSheet( "QLabel { color : green; }" );
            break;

        default:
            colorString = "unknown";
            mMainUi->triaColorLabel->setStyleSheet( "QLabel { color : black; }" );
            break;
    }

    mMainUi->triaColorLabel->setText( colorString );

    //***************************************************************************
    // Ethernet Tx
    //***************************************************************************
    mMainUi->txPacketsLabel->setText( QString::number( mModemInfo.TxPackets ) );

    QString amount;
    QString units;
    
    if( mModemInfo.TxBytes >= ONE_GB )
    {
        amount = QString::number( mModemInfo.TxBytes / ONE_GB, 'f', 3 );
        units = "GBytes";
    }
    else if( mModemInfo.TxBytes >= ONE_MB )
    {
        amount = QString::number( mModemInfo.TxBytes / ONE_MB, 'f', 3 );
        units = "MBytes";
    }
    else if( mModemInfo.TxBytes >= ONE_KB )
    {
        amount = QString::number( mModemInfo.TxBytes / ONE_KB, 'f', 3 );
        units = "kBytes";
    }
    else
    {
        amount = QString::number( mModemInfo.TxBytes );
        units = "Bytes";
    }
    
    mMainUi->txBytesLabel->setText( amount );
    mMainUi->txBytesStatic->setText( units );

    //***************************************************************************
    // Ethernet Rx
    //***************************************************************************
    mMainUi->rxPacketsLabel->setText( QString::number( mModemInfo.RxPackets ) );

    if( mModemInfo.RxBytes >= ONE_GB )
    {
        amount = QString::number( mModemInfo.RxBytes / ONE_GB, 'f', 3 );
        units = "GBytes";
    }
    else if( mModemInfo.RxBytes >= ONE_MB )
    {
        amount = QString::number( mModemInfo.RxBytes / ONE_MB, 'f', 3 );
        units = "MBytes";
    }
    else if( mModemInfo.RxBytes >= ONE_KB )
    {
        amount = QString::number( mModemInfo.RxBytes / ONE_KB, 'f', 3 );
        units = "kBytes";
    }
    else
    {
        amount = QString::number( mModemInfo.RxBytes );
        units = "Bytes";
    }
    
    mMainUi->rxBytesLabel->setText( amount );
    mMainUi->rxBytesStatic->setText( units );

    //***************************************************************************
    // RF Rx
    //***************************************************************************
    mMainUi->rxSnrLabel->setText( QString::number( mModemInfo.RxSnrDb, 'f', 1 ) + " dB" );
    mMainUi->rxSnrProgressbar->setValue( mModemInfo.RxSnrPercent );

    if( mModemInfo.RxSnrDb >= 10 )
    {
        mMainUi->rxSnrProgressbar->setStyleSheet(
        "QProgressBar { background-color: rgb( 191, 191, 191 ); border-radius: 5px; } QProgressBar::chunk { background-color: rgb( 0, 191, 0 ); border-radius: 5px; }" );
    }
    else if( mModemInfo.RxSnrDb >= 7 )
    {
        mMainUi->rxSnrProgressbar->setStyleSheet(
        "QProgressBar { background-color: rgb( 191, 191, 191 ); border-radius: 5px; } QProgressBar::chunk { background-color: rgb( 223, 223, 0 ); border-radius: 5px; }" );
    }
    else if( mModemInfo.RxSnrDb >= 4 )
    {
        mMainUi->rxSnrProgressbar->setStyleSheet(
        "QProgressBar { background-color: rgb( 191, 191, 191 ); border-radius: 5px; } QProgressBar::chunk { background-color: rgb( 255, 127, 0 ); border-radius: 5px; }" );
    }
    else
    {
        mMainUi->rxSnrProgressbar->setStyleSheet(
        "QProgressBar { background-color: rgb( 191, 191, 191 ); border-radius: 5px; } QProgressBar::chunk { background-color: rgb( 223, 0, 0 ); border-radius: 5px; } " );
    }
    
    mMainUi->rxRfPowerLabel->setText( QString::number( mModemInfo.RxPwrDbm, 'f', 1 ) + " dBm / " + convertDbmToQstring( mModemInfo.RxPwrDbm ) );
    mMainUi->rxRfPowerProgressbar->setValue( mModemInfo.RxPwrPercent );

    //***************************************************************************
    // RF Tx
    //***************************************************************************
    mMainUi->txIfPowerLabel->setText( QString::number( mTriaInfo.TxIfPwrDbm, 'f', 1 ) + " dBm / " + convertDbmToQstring( mTriaInfo.TxIfPwrDbm ) );
    mMainUi->txIfPowerProgressbar->setValue( mTriaInfo.TxIfPwrPercent );

    mMainUi->txRfPowerLabel->setText( QString::number( mTriaInfo.TxRfPwrDbm, 'f', 1 ) + " dBm / " + convertDbmToQstring( mTriaInfo.TxRfPwrDbm ) );
    mMainUi->txRfPowerProgressbar->setValue( mTriaInfo.TxRfPwrPercent );

    //***************************************************************************
    // Cable
    //***************************************************************************
    mMainUi->cableAttenuationLabel->setText( QString::number( mModemInfo.CableAttenuationDb, 'f', 1 ) + " dB" );
    mMainUi->cableAttenuationProgressbar->setValue( mModemInfo.CableAttenuationPercent );

    mMainUi->cableResistanceLabel->setText( QString::number( mModemInfo.CableResistanceOhm, 'f', 1 ) + " " + OMEGA_CAPITAL );
    mMainUi->cableResistanceProgressbar->setValue( mModemInfo.CableResistancePercent );
}

//!************************************************************************
//! Update the modem information
//!
//! @returns: nothing
//!************************************************************************
void SurfBeam2::updateModemInfo()
{
    for( int i = 0; i < mModemRawStringsList.size(); i++ )
    {
        switch( i )
        {
            case MODEM_INDEX_IP_ADDRESS:
                mModemInfo.IpAddress = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_MAC_ADDRESS:
                mModemInfo.MacAddress = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_SW_VERSION:
                mModemInfo.SwVersion = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_HW_VERSION:
                mModemInfo.HwVersion = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_STATUS:
                mModemInfo.ModemStatusLabel = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_TX_PACKETS:
                {
                    QString rawString = mModemRawStringsList.at( i );
                    rawString.remove( ',' );
                    mModemInfo.TxPackets = rawString.toULongLong();
                }
                break;

            case MODEM_INDEX_TX_BYTES:
                {
                    QString rawString = mModemRawStringsList.at( i );
                    rawString.remove( ',' );
                    mModemInfo.TxBytes = rawString.toULongLong();
                }
                break;

            case MODEM_INDEX_RX_PACKETS:
                {
                    QString rawString = mModemRawStringsList.at( i );
                    rawString.remove( ',' );
                    mModemInfo.RxPackets = rawString.toULongLong();
                }
                break;

            case MODEM_INDEX_RX_BYTES:
                {
                    QString rawString = mModemRawStringsList.at( i );
                    rawString.remove( ',' );
                    mModemInfo.RxBytes = rawString.toULongLong();
                }
                break;

            case MODEM_INDEX_ONLINE_TIME:
                mModemInfo.OnlineTime = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_LOSS_OF_SYNC_COUNT:
                mModemInfo.LossOfSyncCount = mModemRawStringsList.at( i ).toULongLong();
                break;

            case MODEM_INDEX_RX_SNR_DB:
                mModemInfo.RxSnrDb = mModemRawStringsList.at( i ).toDouble();
                break;

            case MODEM_INDEX_RX_SNR_PERCENT:
                {
                    QString rawString = mModemRawStringsList.at( i );
                    rawString.remove( '%' );
                    mModemInfo.RxSnrPercent = rawString.toUShort();
                }
                break;

            case MODEM_INDEX_SERIAL_NR:
                mModemInfo.SerialNumber = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_RX_PWR_DBM:
                mModemInfo.RxPwrDbm = mModemRawStringsList.at( i ).toDouble();
                break;

            case MODEM_INDEX_RX_PWR_PERCENT:
                {
                    QString rawString = mModemRawStringsList.at( i );
                    rawString.remove( '%' );
                    mModemInfo.RxPwrPercent = rawString.toUShort();
                }
                break;

            case MODEM_INDEX_CABLE_RESISTANCE_OHM:
                mModemInfo.CableResistanceOhm = mModemRawStringsList.at( i ).toDouble();
                break;

            case MODEM_INDEX_CABLE_RESISTANCE_PERCENT:
                {
                    QString rawString = mModemRawStringsList.at( i );
                    rawString.remove( '%' );
                    mModemInfo.CableResistancePercent = rawString.toUShort();
                }
                break;

            case MODEM_INDEX_ODU_TELEMETRY_STATUS:
                mModemInfo.OutdoorUnitTelemetryStatus = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_CABLE_ATTEN_DB:
                mModemInfo.CableAttenuationDb = mModemRawStringsList.at( i ).toDouble();
                break;

            case MODEM_INDEX_CABLE_ATTEN_PERCENT:
                {
                    QString rawString = mModemRawStringsList.at( i );
                    rawString.remove( '%' );
                    mModemInfo.CableAttenuationPercent = rawString.toUShort();
                }
                break;

            case MODEM_INDEX_IFL_TYPE:
                mModemInfo.InterFacilityLinkType = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_PART_NR:
                mModemInfo.PartNr = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_MODEM_STATUS:
                {
                    QString stateString = mModemRawStringsList.at( i );

                    if( stateString.contains( "scanning", Qt::CaseInsensitive ) )
                    {
                         mModemInfo.ModemStatus = MODEM_STATE_SCANNING;
                    }
                    else if( stateString.contains( "ranging", Qt::CaseInsensitive ) )
                    {
                         mModemInfo.ModemStatus = MODEM_STATE_RANGING;
                    }
                    else if( stateString.contains( "network", Qt::CaseInsensitive ) )
                    {
                         mModemInfo.ModemStatus = MODEM_STATE_NETWORK_ENTRY;
                    }
                    else if( stateString.contains( "dhcp", Qt::CaseInsensitive ) )
                    {
                         mModemInfo.ModemStatus = MODEM_STATE_DHCP;
                    }
                    else if( stateString.contains( "online", Qt::CaseInsensitive ) )
                    {
                         mModemInfo.ModemStatus = MODEM_STATE_ONLINE;
                    }
                    else
                    {
                        mModemInfo.ModemStatus = MODEM_STATE_UNKNOWN;
                    }
                }
                break;

            case MODEM_INDEX_SATELLITE_STATUS:
                {
                    QString beamColorString = mModemRawStringsList.at( i );

                    if( beamColorString.contains( "blue", Qt::CaseInsensitive ) )
                    {
                         mModemInfo.SatStatusBeamColor = SATELLITE_STATUS_BEAM_COLOR_BLUE;
                    }
                    else if( beamColorString.contains( "orange", Qt::CaseInsensitive ) )
                    {
                         mModemInfo.SatStatusBeamColor = SATELLITE_STATUS_BEAM_COLOR_ORANGE;
                    }
                    else if( beamColorString.contains( "purple", Qt::CaseInsensitive ) )
                    {
                         mModemInfo.SatStatusBeamColor = SATELLITE_STATUS_BEAM_COLOR_PURPLE;
                    }
                    else if( beamColorString.contains( "green", Qt::CaseInsensitive ) )
                    {
                         mModemInfo.SatStatusBeamColor = SATELLITE_STATUS_BEAM_COLOR_GREEN;
                    }
                    else
                    {
                        mModemInfo.SatStatusBeamColor = SATELLITE_STATUS_BEAM_COLOR_UNKNOWN;
                    }
                }
                break;

            case MODEM_INDEX_CLIENT_SIDE_PROXY_STATUS:
                mModemInfo.ClientSideProxyStatus = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_CLIENT_SIDE_PROXY_HEALTH:
                mModemInfo.ClientSideProxyHealth = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_LAST_PAGE_LOAD_DURATION:
                mModemInfo.LastPageLoadDuration = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_UPLINK_SYMBOL_RATE:
                mModemInfo.UplinkSymbolRate = mModemRawStringsList.at( i ).toULong();
                break;

            case MODEM_INDEX_BDT_VERSION:
                mModemInfo.BeamDataTableVersion = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_VENDOR:
                mModemInfo.Vendor = mModemRawStringsList.at( i );
                break;

            case MODEM_INDEX_DOWNLINK_SYMBOL_RATE:
                mModemInfo.DownlinkSymbolRate = mModemRawStringsList.at( i ).toULong();
                break;

            case MODEM_INDEX_DOWNLINK_MODULATION:
                mModemInfo.DownlinkModulation = mModemRawStringsList.at( i );
                break;

            default:
                break;
        }
    }
}

//!************************************************************************
//! Update the TRIA information
//!
//! @returns: nothing
//!************************************************************************
void SurfBeam2::updateTriaInfo()
{
    for( int i = 0; i < mTriaRawStringsList.size(); i++ )
    {
        switch( i )
        {
            case TRIA_INDEX_PWR_MODE:
                mTriaInfo.PwrMode = mTriaRawStringsList.at( i );
                break;

            case TRIA_INDEX_POLARIZATION_TYPE:
                mTriaInfo.PolarizationType = mTriaRawStringsList.at( i );
                break;

            case TRIA_INDEX_TX_IF_PWR_DBM:
                mTriaInfo.TxIfPwrDbm = mTriaRawStringsList.at( i ).toDouble();
                break;

            case TRIA_INDEX_IFL_TYPE:
                mTriaInfo.InterFacilityLinkType = mTriaRawStringsList.at( i );
                break;

            case TRIA_INDEX_TEMPERATURE_C:
                mTriaInfo.TemperatureCelsius = mTriaRawStringsList.at( i ).toDouble();
                break;

            case TRIA_INDEX_SERIAL_NR:
                mTriaInfo.SerialNumber = mTriaRawStringsList.at( i );
                break;

            case TRIA_INDEX_TX_RF_PWR_DBM:
                mTriaInfo.TxRfPwrDbm = mTriaRawStringsList.at( i ).toDouble();
                break;

            case TRIA_INDEX_FW_VERSION:
                mTriaInfo.FwVersion = mTriaRawStringsList.at( i );
                break;

            case TRIA_INDEX_TX_IF_PWR_PERCENT:
                {
                    QString rawString = mTriaRawStringsList.at( i );
                    rawString.remove( '%' );
                    mTriaInfo.TxIfPwrPercent = rawString.toUShort();
                }
                break;

            case TRIA_INDEX_TX_RF_PWR_PERCENT:
                {
                    QString rawString = mTriaRawStringsList.at( i );
                    rawString.remove( '%' );
                    mTriaInfo.TxRfPwrPercent = rawString.toUShort();
                }
                break;

            case TRIA_INDEX_SATELLITE_STATUS:
                {
                    QString beamColorString = mTriaRawStringsList.at( i );

                    if( beamColorString.contains( "blue", Qt::CaseInsensitive ) )
                    {
                         mTriaInfo.SatStatusBeamColor = SATELLITE_STATUS_BEAM_COLOR_BLUE;
                    }
                    else if( beamColorString.contains( "orange", Qt::CaseInsensitive ) )
                    {
                         mTriaInfo.SatStatusBeamColor = SATELLITE_STATUS_BEAM_COLOR_ORANGE;
                    }
                    else if( beamColorString.contains( "purple", Qt::CaseInsensitive ) )
                    {
                         mTriaInfo.SatStatusBeamColor = SATELLITE_STATUS_BEAM_COLOR_PURPLE;
                    }
                    else if( beamColorString.contains( "green", Qt::CaseInsensitive ) )
                    {
                         mTriaInfo.SatStatusBeamColor = SATELLITE_STATUS_BEAM_COLOR_GREEN;
                    }
                    else
                    {
                        mTriaInfo.SatStatusBeamColor = SATELLITE_STATUS_BEAM_COLOR_UNKNOWN;
                    }
                }
                break;

            case TRIA_INDEX_VENDOR:
                mTriaInfo.Vendor = mTriaRawStringsList.at( i );
                break;

            default:
                break;
        }
    }
} 
