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
SurfBeam2.h

This file contains the definitions for the ViaSat SurfBeam 2 modem.
*/

#ifndef SurfBeam2_h
#define SurfBeam2_h

#include <cstdint>
#include <vector>

#include <QByteArray>
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QString>
#include <QStringList>
#include <QUrl>


QT_BEGIN_NAMESPACE

namespace Ui
{
    class SurfBeam2;
}

QT_END_NAMESPACE

//************************************************************************
// Class for handling the information from the ViaSat SurfBeam 2 satellite modem
//************************************************************************
class SurfBeam2 : public QMainWindow
{
    Q_OBJECT

    //************************************************************************
    // constants and types
    //************************************************************************
    private:
        const QUrl URL_MODEM = QUrl( "http://192.168.100.1/index.cgi?page=modemStatusData" );   //!< modem CGI URL
        const QUrl URL_TRIA  = QUrl( "http://192.168.100.1/index.cgi?page=triaStatusData" );    //!< TRIA CGI URL

        const uint8_t FIELD_COUNT_MODEM = 81;   //!< number of fields in the modem string array (matches fw ver. UT_3.7.8.9.5)
        const uint8_t FIELD_COUNT_TRIA  = 84;   //!< number of fields in the TRIA string array (matches fw ver. UT_3.7.8.9.5)

        const QString FIELD_DELIMITER = "##";   //!< field delimiter
        const QString FIELD_FILL = "#";         //!< filling character

        const QString OMEGA_CAPITAL = QString::fromUtf8( "\u03A9" );    //!< capital Greek Omega
        const QString MU_SMALL = QString::fromUtf8( "\u03BC" );         //!< small Greek mu

        static constexpr double ONE_KB = 1024.0;                        //!< bytes in one kB
        static constexpr double ONE_MB = ONE_KB * ONE_KB;               //!< bytes in one MB
        static constexpr double ONE_GB = ONE_KB * ONE_MB;               //!< bytes in one GB

        enum SatelliteStatusBeamColor
        {
            SATELLITE_STATUS_BEAM_COLOR_UNKNOWN,    //!< unknown or uninitialized

            SATELLITE_STATUS_BEAM_COLOR_BLUE,       //!< blue beam color
            SATELLITE_STATUS_BEAM_COLOR_ORANGE,     //!< orange beam color
            SATELLITE_STATUS_BEAM_COLOR_PURPLE,     //!< purple beam color
            SATELLITE_STATUS_BEAM_COLOR_GREEN,      //!< green beam color

            SATELLITE_STATUS_BEAM_COLOR_COUNT       //!< number of defined beam colors
        };

        enum ModemState
        {
            MODEM_STATE_UNKNOWN,        //!< unknown or uninitialized

            MODEM_STATE_SCANNING,       //!< Scanning        - step 1 of 5
            MODEM_STATE_RANGING,        //!< Ranging         - step 2 of 5
            MODEM_STATE_NETWORK_ENTRY,  //!< Network entry   - step 3 of 5
            MODEM_STATE_DHCP,           //!< DHCP            - step 4 of 5
            MODEM_STATE_ONLINE,         //!< Online          - step 5 of 5

            MODEM_STATE_COUNT           //!< Number of defined modem states
        };

        enum ModemIndexes
        {
            MODEM_INDEX_IP_ADDRESS              = 0,    //!< IPv4 address
            MODEM_INDEX_MAC_ADDRESS             = 1,    //!< MAC address
            MODEM_INDEX_SW_VERSION              = 2,    //!< software version
            MODEM_INDEX_HW_VERSION              = 3,    //!< hardware version
            MODEM_INDEX_STATUS                  = 4,    //!< status
            MODEM_INDEX_RX_PACKETS              = 5,    //!< number of received packets
            MODEM_INDEX_RX_BYTES                = 6,    //!< number of received bytes
            MODEM_INDEX_TX_PACKETS              = 7,    //!< number of transmitted packets
            MODEM_INDEX_TX_BYTES                = 8,    //!< number of transmitted bytes
            MODEM_INDEX_ONLINE_TIME             = 9,    //!< online time
            MODEM_INDEX_LOSS_OF_SYNC_COUNT      = 10,   //!< loss-of-sync count
            MODEM_INDEX_RX_SNR_DB               = 11,   //!< Rx SNR [dB]
            MODEM_INDEX_RX_SNR_PERCENT          = 12,   //!< Rx SNR [%]
            MODEM_INDEX_SERIAL_NR               = 13,   //!< serial number
            MODEM_INDEX_RX_PWR_DBM              = 14,   //!< Rx power [dBm]
            MODEM_INDEX_RX_PWR_PERCENT          = 15,   //!< Rx power [%]
            MODEM_INDEX_CABLE_RESISTANCE_OHM    = 16,   //!< cable resistance [Ohm]
            MODEM_INDEX_CABLE_RESISTANCE_PERCENT= 17,   //!< cable resistance [%]
            MODEM_INDEX_ODU_TELEMETRY_STATUS    = 18,   //!< ODU telemetry status
            MODEM_INDEX_CABLE_ATTEN_DB          = 19,   //!< cable attenuation [dB]
            MODEM_INDEX_CABLE_ATTEN_PERCENT     = 20,   //!< cable attenuation [%]
            MODEM_INDEX_IFL_TYPE                = 21,   //!< IFL type
            MODEM_INDEX_PART_NR                 = 22,   //!< part number
            MODEM_INDEX_MODEM_STATUS            = 23,   //!< modem status
            MODEM_INDEX_SATELLITE_STATUS        = 24,   //!< satellite status
            //
            MODEM_INDEX_CLIENT_SIDE_PROXY_STATUS= 26,   //!< client-side proxy status
            MODEM_INDEX_CLIENT_SIDE_PROXY_HEALTH= 27,   //!< client-side proxy health
            //
            MODEM_INDEX_LAST_PAGE_LOAD_DURATION = 30,   //!< last page load duration
            //
            MODEM_INDEX_UPLINK_SYMBOL_RATE      = 32,   //!< uplink SR
            //
            MODEM_INDEX_BDT_VERSION             = 40,   //!< BDT version
            //
            MODEM_INDEX_VENDOR                  = 46,   //!< vendor
            //
            MODEM_INDEX_DOWNLINK_SYMBOL_RATE    = 50,   //!< downlink SR
            MODEM_INDEX_DOWNLINK_MODULATION     = 51    //!< downlink modulation type
        };

        typedef struct
        {
            QString                     IpAddress;
            QString                     MacAddress;
            QString                     SwVersion;
            QString                     HwVersion;
            QString                     ModemStatusLabel;
            uint64_t                    TxPackets;
            uint64_t                    TxBytes;
            uint64_t                    RxPackets;
            uint64_t                    RxBytes;
            QString                     OnlineTime;
            uint32_t                    LossOfSyncCount;
            double                      RxSnrDb;
            uint8_t                     RxSnrPercent;
            QString                     SerialNumber;
            double                      RxPwrDbm;
            uint8_t                     RxPwrPercent;
            double                      CableResistanceOhm;
            uint8_t                     CableResistancePercent;
            QString                     OutdoorUnitTelemetryStatus;
            double                      CableAttenuationDb;
            uint8_t                     CableAttenuationPercent;
            QString                     InterFacilityLinkType;
            QString                     PartNr;
            ModemState                  ModemStatus;
            SatelliteStatusBeamColor    SatStatusBeamColor;
            QString                     ClientSideProxyStatus;
            QString                     ClientSideProxyHealth;
            QString                     LastPageLoadDuration;
            uint32_t                    UplinkSymbolRate;
            QString                     BeamDataTableVersion;
            QString                     Vendor;
            uint32_t                    DownlinkSymbolRate;
            QString                     DownlinkModulation;
        }ModemInfo;

        enum TriaIndexes
        {
            TRIA_INDEX_PWR_MODE                 = 4,    //!< power mode
            TRIA_INDEX_POLARIZATION_TYPE        = 5,    //!< polarization type
            //
            TRIA_INDEX_TX_IF_PWR_DBM            = 7,    //!< Tx IF power [dBm]
            //
            TRIA_INDEX_IFL_TYPE                 = 9,    //!< IFL type
            TRIA_INDEX_TEMPERATURE_C            = 10,   //!< temperature [C]
            //
            TRIA_INDEX_SERIAL_NR                = 16,   //!< serial number
            TRIA_INDEX_TX_RF_PWR_DBM            = 17,   //!< Tx RF power [dBm]
            //
            TRIA_INDEX_FW_VERSION               = 24,   //!< firmware version
            TRIA_INDEX_TX_IF_PWR_PERCENT        = 25,   //!< Tx IF power [%]
            TRIA_INDEX_TX_RF_PWR_PERCENT        = 26,   //!< Tx RF power [%]
            //
            TRIA_INDEX_SATELLITE_STATUS         = 29,   //!< satellite status
            //
            TRIA_INDEX_VENDOR                   = 81    //!< vendor
        };

        typedef struct
        {
            QString                     PwrMode;
            QString                     PolarizationType;
            double                      TxIfPwrDbm;
            QString                     InterFacilityLinkType;
            double                      TemperatureCelsius;
            QString                     SerialNumber;
            double                      TxRfPwrDbm;
            QString                     FwVersion;
            uint8_t                     TxIfPwrPercent;
            uint8_t                     TxRfPwrPercent;
            SatelliteStatusBeamColor    SatStatusBeamColor;
            QString                     Vendor;
        }TriaInfo;

    //************************************************************************
    // functions
    //************************************************************************
    public:
        SurfBeam2
            (
            QWidget* aParent = nullptr  //!< a parent widget
            );

        ~SurfBeam2();

    private:
        double convertDbmToWatts
            (
            const double aDbm           //!< power in dBm
            );

        QString convertDbmToQstring
            (
            const double aDbm           //!< power in dBm
            );


        double getCableAttenuationPercent
            (
            const double aCableAttenuationDb    //!< attenuation in dB
            );

        double getRxPwrPercent
            (
            const double aRxPwrDbm      //!< power in dBm
            );

        double getRxSnrPercent
            (
            const double aRxSnrDb       //!< SNR in dB
            );

        double getTxIfPwrPercent
            (
            const double aTxIfPwrDbm    //!< power in dBm
            );

        double getTxRfPwrPercent
            (
            const double aTxRfPwrDbm    //!< power in dBm
            );

        void updateContent();

        void updateModemInfo();

        void updateTriaInfo();

    private slots:
        void httpFinishedModem();
        void httpFinishedTria();

        void httpReadyReadModem();
        void httpReadyReadTria();

        void startCgiRequest();


    //************************************************************************
    // variables
    //************************************************************************
    private:
        Ui::SurfBeam2*          mMainUi;                //!< main UI

        QStringList             mModemRawStringsList;   //!< raw strings list with modem items
        QStringList             mTriaRawStringsList;    //!< raw strings list with TRIA items

        ModemInfo               mModemInfo;             //!< object with modem information
        TriaInfo                mTriaInfo;              //!< object with TRIA information

        QNetworkAccessManager   mQnam;                  //!< network access manager

        QNetworkReply*          mReplyModem;            //!< modem network reply
        QNetworkReply*          mReplyTria;             //!< TRIA network reply

        QByteArray              mByteArrayModem;        //!< modem byte array
        QByteArray              mByteArrayTria;         //!< TRIA byte array
};
#endif // SurfBeam2_h
