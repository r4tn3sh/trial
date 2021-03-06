/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2015 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02300-1307  USA
 *
 * Created on: Aug 11, 2015
 * Authors: Biljana Bojovic <bbojovic@cttc.es> and Tom Henderson <tomh@tomh.org>
 */


#include "lbt-access-manager.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/spectrum-wifi-phy.h"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("LbtAccessManager");

    NS_OBJECT_ENSURE_REGISTERED (LbtAccessManager);


    /**
     * Listener for PHY events. Forwards to lbtaccessmanager
     */
    class LbtPhyListener : public ns3::WifiPhyListener
    {
        public:
            /**
             * Create a PhyListener for the given DcfManager.
             *
             * \param dcf
             */
            LbtPhyListener (ns3::LbtAccessManager *lbt) : m_lbtAccessManager (lbt)
        {
        }

            virtual ~LbtPhyListener ()
            {
            }

            virtual void NotifyRxStart (Time duration)
            {
                NS_LOG_FUNCTION (this << duration);
                m_lbtAccessManager->NotifyRxStartNow (duration);
            }
            virtual void NotifyRxEndOk (void)
            {
                m_lbtAccessManager->NotifyRxEndOkNow ();
            }
            virtual void NotifyRxEndError (void)
            {
                m_lbtAccessManager->NotifyRxEndErrorNow ();
            }
            virtual void NotifyTxStart (Time duration, double txPowerDbm)
            {
                NS_LOG_FUNCTION (this << duration << txPowerDbm);
                m_lbtAccessManager->NotifyTxStartNow (duration);
            }
            virtual void NotifyMaybeCcaBusyStart (Time duration)
            {
                NS_LOG_FUNCTION (this << duration);
                m_lbtAccessManager->NotifyMaybeCcaBusyStartNow (duration);
            }
            virtual void NotifySwitchingStart (Time duration)
            {
                NS_LOG_FUNCTION (this << duration);
                m_lbtAccessManager->NotifySwitchingStartNow (duration);
            }
            virtual void NotifySleep (void)
            {
                m_lbtAccessManager->NotifySleepNow ();
            }
            virtual void NotifyWakeup (void)
            {
                m_lbtAccessManager->NotifyWakeupNow ();
            }

        private:
            ns3::LbtAccessManager *m_lbtAccessManager;  //!< DcfManager to forward events to
    };

    /**
     * Listener for NAV events. Forwards to LbtAccessManager
     */
    class LbtMacLowListener : public ns3::MacLowDcfListener
    {
        public:
            /**
             * Create a LowDcfListener for the given DcfManager.
             *
             * \param dcf
             */
            LbtMacLowListener (ns3::LbtAccessManager *dcf)
                : m_lbtAccessManager (dcf)
            {
            }
            virtual ~LbtMacLowListener ()
            {
            }
            virtual void NavStart (Time duration)
            {
                m_lbtAccessManager->NotifyNavStartNow (duration);
            }
            virtual void NavReset (Time duration)
            {
                m_lbtAccessManager->NotifyNavResetNow (duration);
            }
            virtual void AckTimeoutStart (Time duration)
            {
                m_lbtAccessManager->NotifyAckTimeoutStartNow (duration);
            }
            virtual void AckTimeoutReset ()
            {
                m_lbtAccessManager->NotifyAckTimeoutResetNow ();
            }
            virtual void CtsTimeoutStart (Time duration)
            {
                m_lbtAccessManager->NotifyCtsTimeoutStartNow (duration);
            }
            virtual void CtsTimeoutReset ()
            {
                m_lbtAccessManager->NotifyCtsTimeoutResetNow ();
            }
        private:
            ns3::LbtAccessManager *m_lbtAccessManager;    //!< DcfManager to forward events to
    };



    TypeId
        LbtAccessManager::GetTypeId (void)
        {
            static TypeId tid = TypeId ("ns3::LbtAccessManager")
                .SetParent<ChannelAccessManager> ()
                .SetGroupName ("laa-wifi-coexistence")
                .AddAttribute ("Slot", "The duration of a Slot.",
                        TimeValue (MicroSeconds (20)),
                        MakeTimeAccessor (&LbtAccessManager::m_slotTime),
                        MakeTimeChecker ())
                .AddAttribute ("DeferTime", "TimeInterval to defer during CCA",
                        TimeValue (MicroSeconds (43)),
                        MakeTimeAccessor (&LbtAccessManager::m_deferTime),
                        MakeTimeChecker ())
                .AddAttribute ("MinCw", "The minimum value of the contention window.",
                        UintegerValue (15),
                        MakeUintegerAccessor (&LbtAccessManager::m_cwMin),
                        MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("MaxCw", "The maximum value of the contention window. For the priority class 3 this value is set to 63, and for priority class 4 it is 1023.",
                        //UintegerValue (63),
                        UintegerValue (1023),
                        MakeUintegerAccessor (&LbtAccessManager::m_cwMax),
                        MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("CwFactor", "Multiplication factor",
                        UintegerValue (2),
                        MakeUintegerAccessor (&LbtAccessManager::m_cwFactor),
                        MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("Txop",
                        "Duration of channel access grant.",
                        TimeValue (MilliSeconds (8)),
                        MakeTimeAccessor (&LbtAccessManager::m_txop),
                        MakeTimeChecker (MilliSeconds (1), MilliSeconds (20)))
                .AddAttribute ("UseReservationSignal",
                        "Whether to use a reservation signal when there is no data ready to be transmitted.",
                        BooleanValue (true),
                        MakeBooleanAccessor (&LbtAccessManager::m_reservationSignal),
                        MakeBooleanChecker ())
                .AddAttribute ("CwUpdateRule",
                        "Rule according which CW will be updated",
                        EnumValue (LbtAccessManager::NACKS_80_PERCENT),
                        MakeEnumAccessor (&LbtAccessManager::m_cwUpdateRule),
                        MakeEnumChecker (LbtAccessManager::ALL_NACKS, "ALL_NACKS",
                            LbtAccessManager::ANY_NACK, "ANY_NACK",
                            LbtAccessManager::NACKS_10_PERCENT, "NACKS_10_PERCENT",
                            LbtAccessManager::NACKS_80_PERCENT, "NACKS_80_PERCENT"))
                .AddTraceSource ("Cw",
                        "CW change trace source; logs changes to CW",
                        MakeTraceSourceAccessor (&LbtAccessManager::m_cw),
                        "ns3::TracedValue::Uint32Callback")
                .AddTraceSource ("Backoff",
                        "Backoff change trace source; logs any new backoff",
                        MakeTraceSourceAccessor (&LbtAccessManager::m_currentBackoffSlots),
                        "ns3::TracedValue::Uint32Callback")
                .AddAttribute ("HarqFeedbackDelay", "Harq feedback delay",
                        TimeValue (MilliSeconds (5)),
                        MakeTimeAccessor (&LbtAccessManager::m_harqFeedbackDelay),
                        MakeTimeChecker ())
                .AddAttribute ("HarqFeedbackExpirationTime", "If harq feedback is older than this time,  do not use it for contention windown update.",
                        TimeValue (MilliSeconds (100)),
                        MakeTimeAccessor (&LbtAccessManager::m_harqFeedbackExpirationTime),
                        MakeTimeChecker ())
                .AddAttribute ("ChannelSenseMode",
                        "Channel sensing mode used",
                        EnumValue (LbtAccessManager::ENE),
                        MakeEnumAccessor (&LbtAccessManager::m_channelSenseMode),
                        MakeEnumChecker (LbtAccessManager::ENE, "ENE",
                            LbtAccessManager::ENECTS, "ENECTS",
                            LbtAccessManager::PRE, "PRE",
                            LbtAccessManager::PRECTS, "PRECTS"))
                .AddConstructor<LbtAccessManager> ()
                ;
            return tid;
        }


    LbtAccessManager::LbtAccessManager ()
        : ChannelAccessManager (),
        m_lbtPhyListener (0),
        m_lbtMacLowListener (0),
        m_state (IDLE),
        m_currentBackoffSlots (0),
        m_backoffCount (0),
        m_grantRequested (false),
        m_useCtsToSelf (false),
        m_backoffStartTime (Seconds (0)),
        m_lastBusyTime (Seconds (0)),
        m_lastReqStartTime (Seconds (0))  //!< XXX: Ratnesh
    {
        NS_LOG_FUNCTION (this);
        m_rng = CreateObject<UniformRandomVariable> ();
    }

    LbtAccessManager::~LbtAccessManager ()
    {
        NS_LOG_FUNCTION (this);
        // TODO Auto-generated destructor stub
        delete m_lbtPhyListener;
        delete m_lbtMacLowListener;
        m_lbtPhyListener = 0;
        m_lbtMacLowListener = 0;
        if (m_waitForDeferEventId.IsRunning ())
        {
            m_waitForDeferEventId.Cancel ();
        }
        if (m_waitForBackoffEventId.IsRunning ())
        {
            m_waitForBackoffEventId.Cancel ();
        }
        if (m_waitForBusyEventId.IsRunning ())
        {
            m_waitForBusyEventId.Cancel ();
        }
        if (m_waitForCtsEventId.IsRunning ())
        {
            m_waitForCtsEventId.Cancel ();
        }
    }

    void
        LbtAccessManager::EnableCtsToSelf()
        {
            NS_LOG_FUNCTION (this);
            m_useCtsToSelf = true;
        }
        
    int64_t
        LbtAccessManager::AssignStreams (int64_t stream)
        {
            NS_LOG_FUNCTION (this << stream);
            m_rng->SetStream (stream);
            return 1;
        }

    void
        LbtAccessManager::SetupPhyListener (Ptr<SpectrumWifiPhy> phy)
        {
            NS_LOG_FUNCTION (this << phy);
            if (m_lbtPhyListener != 0)
            {
                NS_LOG_DEBUG ("Deleting previous listener " << m_lbtPhyListener);
                phy->UnregisterListener (m_lbtPhyListener);
                delete m_lbtPhyListener;
            }
            m_lbtPhyListener = new LbtPhyListener (this);
            phy->RegisterListener (m_lbtPhyListener);
        }

    void
        LbtAccessManager::SetupLowListener (Ptr<MacLow> low)
        {
            NS_LOG_FUNCTION (this << low);
            m_wifiMacLow = low;
            m_wifiMacLow->SetCtsToSelfSupported(true);
            m_wifiMacLow->SetLaaWifi();
            if (m_lbtMacLowListener != 0)
            {
                delete m_lbtMacLowListener;
            }
            m_lbtMacLowListener = new LbtMacLowListener (this);
            low->RegisterDcfListener (m_lbtMacLowListener);
        }

    void
        LbtAccessManager::SetWifiPhy (Ptr<SpectrumWifiPhy> phy)
        {
            NS_LOG_FUNCTION (this << phy);
            SetupPhyListener (phy);
            m_wifiPhy = phy;
            // Configure the WifiPhy to treat each incoming signal as a foreign signal
            // (energy detection only)
            if (m_channelSenseMode == ENE || m_channelSenseMode == ENECTS)
            {
                m_wifiPhy->SetAttribute ("DisableWifiReception", BooleanValue (true));
            }
            m_wifiPhy->SetAttribute ("CcaMode1Threshold", DoubleValue (m_edThreshold));

            // Initialization of post-attribute-construction variables can be done here
            m_cw = m_cwMin;
        }

    void
        LbtAccessManager::SetLteEnbMac (Ptr<LteEnbMac> lteEnbMac)
        {
            NS_LOG_FUNCTION (this << lteEnbMac);
            m_lteEnbMac = lteEnbMac;
            m_lteEnbMac->TraceConnectWithoutContext ("DlHarqFeedback", MakeCallback (&ns3::LbtAccessManager::UpdateCwBasedOnHarq,this));
        }

    void
        LbtAccessManager::SetLteEnbPhy (Ptr<LteEnbPhy> lteEnbPhy)
        {
            NS_LOG_FUNCTION (this << lteEnbPhy);
            m_lteEnbPhy = lteEnbPhy;
            m_lteEnbPhy->SetUseReservationSignal (m_reservationSignal);
        }


    LbtAccessManager::LbtState
        LbtAccessManager::GetLbtState () const
        {
            NS_LOG_FUNCTION (this);
            return m_state;
        }

    void
        LbtAccessManager::SetLbtState (LbtAccessManager::LbtState state)
        {
            NS_LOG_FUNCTION (this << state);
            m_state = state;
        }

    void
        LbtAccessManager::DoRequestAccess ()
        {
            NS_LOG_FUNCTION (this);
            NS_ASSERT_MSG (m_wifiPhy, "LbtAccessManager not connected to a WifiPhy");

            if (m_grantRequested == true)
            {
                NS_LOG_LOGIC ("Already waiting to grant access; ignoring request");
                return;
            }

            m_lastReqStartTime = Simulator::Now ();
            if (Simulator::Now () - m_lastBusyTime >= m_deferTime)
            {
                // may grant access immediately according to ETSI BRAN flowchart
                NS_LOG_LOGIC ("Already waited more than defer period since last busy period");
                // XXX: Ratnesh: waiting for CTS transmission before transmitting the subframes
                // if (m_useCtsToSelf)
                if (m_channelSenseMode == PRECTS || m_channelSenseMode == ENECTS)
                {
                    m_wifiMacLow->StartLaaCtsToSelfTx(m_txop- MicroSeconds (45));
                    m_backoffCount = 0;
                    NS_LOG_DEBUG ("Scheduling for CTS");
                    //m_waitForCtsEventId = Simulator::Schedule (Simulator::Now () + MicroSeconds (60), &LbtAccessManager::RequestAccessAfterCts, this);
                    m_waitForCtsEventId = Simulator::Schedule ( MicroSeconds (60), &LbtAccessManager::RequestAccessAfterCts, this);
                    return;
                }
                else
                {
                    SetGrant();
                    return;
                }
            }
            m_grantRequested = true;

            // Draw new backoff value upon entry to the access granting process
            m_currentBackoffSlots = GetBackoffSlots (); // don't decrement traced value
            m_backoffCount = m_currentBackoffSlots; // decrement this counter instead
            NS_LOG_DEBUG ("New backoff count " << m_backoffCount);

            if (GetLbtState () == IDLE)
            {
                // Continue to wait until defer time has expired
                Time deferRemaining = m_deferTime - (Simulator::Now () - m_lastBusyTime);
                NS_LOG_LOGIC ("Must wait " << deferRemaining.GetSeconds () << " defer period");
                m_waitForDeferEventId = Simulator::Schedule (deferRemaining, &LbtAccessManager::RequestAccessAfterDefer, this);
                SetLbtState (WAIT_FOR_DEFER);
            }
            else if (m_lastBusyTime > Simulator::Now ())
            {
                // Access has come in while channel is already busy
                NS_ASSERT_MSG (Simulator::Now () < m_lastBusyTime, "Channel is busy but m_lastBusyTime in the past");
                Time busyRemaining = m_lastBusyTime - Simulator::Now ();
                NS_LOG_LOGIC ("Must wait " << busyRemaining.GetSeconds () << " sec busy period");
                TransitionToBusy (busyRemaining);
            }
        }

    void
        LbtAccessManager::TransitionFromBusy ()
        {
            NS_LOG_FUNCTION (this);
            if (m_lastBusyTime > Simulator::Now ())
            {
                Time busyRemaining = m_lastBusyTime - Simulator::Now ();
                NS_LOG_LOGIC ("Must wait additional " << busyRemaining.GetSeconds () << " busy period");
                m_waitForBusyEventId = Simulator::Schedule (busyRemaining, &LbtAccessManager::TransitionFromBusy, this);
                SetLbtState (BUSY);
            }
            else if (m_grantRequested == true)
            {
                NS_LOG_DEBUG ("Scheduling defer time to expire " << m_deferTime.GetMicroSeconds () + Simulator::Now ().GetMicroSeconds ());
                m_waitForDeferEventId = Simulator::Schedule (m_deferTime, &LbtAccessManager::RequestAccessAfterDefer, this);
                SetLbtState (WAIT_FOR_DEFER);
            }
            else 
            {
                SetLbtState (IDLE);
            }
        }

    void
        LbtAccessManager::TransitionToBusy (Time duration)
        {
            NS_LOG_FUNCTION (this << Simulator::Now () + duration);
            switch (GetLbtState ())
            {
                case IDLE:
                    {
                        NS_ASSERT_MSG (m_backoffCount == 0, "was idle but m_backoff nonzero");
                        m_lastBusyTime = Simulator::Now () + duration;
                        m_waitForBusyEventId = Simulator::Schedule (duration, &LbtAccessManager::TransitionFromBusy, this);
                        NS_LOG_DEBUG ("Going busy until " << m_lastBusyTime.GetMicroSeconds ());
                        SetLbtState (BUSY);
                        break;
                    }
                case TXOP_GRANTED:
                    {
                        m_lastBusyTime = Simulator::Now () + duration;
                        m_waitForBusyEventId = Simulator::Schedule (duration, &LbtAccessManager::TransitionFromBusy, this);
                        NS_LOG_DEBUG ("Going busy until " << m_lastBusyTime.GetMicroSeconds ());
                        SetLbtState (BUSY);
                        break;
                    }
                case BUSY:
                    {
                        // Update last busy time
                        if (m_lastBusyTime < (Simulator::Now () + duration))
                        {
                            NS_LOG_DEBUG ("Update last busy time to " << m_lastBusyTime.GetMicroSeconds ());
                            m_lastBusyTime = Simulator::Now () + duration;
                        }
                        // Since we went busy, a request for access may have come in
                        if (!m_waitForBusyEventId.IsRunning () && m_grantRequested > 0)
                        {
                            Time busyRemaining = m_lastBusyTime - Simulator::Now ();
                            m_waitForBusyEventId = Simulator::Schedule (busyRemaining, &LbtAccessManager::TransitionFromBusy, this);
                            NS_LOG_DEBUG ("Going busy until " << m_lastBusyTime.GetMicroSeconds ());
                            SetLbtState (BUSY);
                        }
                    }
                    break;
                case WAIT_FOR_DEFER:
                    {
                        NS_LOG_DEBUG ("TransitionToBusy from WAIT_FOR_DEFER");
                        // XXX: Ratnesh : Added to handle a corner case
                        // if we are in defer, backoff wait should not already be running
                        if (m_waitForBackoffEventId.IsRunning ())
                        {
                            NS_LOG_DEBUG ("Cancelling Backoff");
                            m_waitForBackoffEventId.Cancel ();
                        }
                        
                        if (m_waitForDeferEventId.IsRunning ())
                        {
                            NS_LOG_DEBUG ("Cancelling Defer");
                            m_waitForDeferEventId.Cancel ();
                        }
                        m_waitForBusyEventId = Simulator::Schedule (duration, &LbtAccessManager::TransitionFromBusy, this);
                        m_lastBusyTime = Simulator::Now () + duration;
                        NS_LOG_DEBUG ("Going busy until " << m_lastBusyTime.GetMicroSeconds ());
                        SetLbtState (BUSY);
                    }
                    break;
                case WAIT_FOR_BACKOFF:
                    {
                        NS_LOG_DEBUG ("TransitionToBusy from WAIT_FOR_BACKOFF");
                        if (m_waitForBackoffEventId.IsRunning ())
                        {
                            NS_LOG_DEBUG ("Cancelling Backoff");
                            m_waitForBackoffEventId.Cancel ();
                        }
                        Time timeSinceBackoffStart;
                        //if (m_useCtsToSelf)
                        if (m_channelSenseMode == PRECTS || m_channelSenseMode == ENECTS)
                        {
                            timeSinceBackoffStart = Simulator::Now () - m_backoffStartTime - MicroSeconds(44); // XXX: Ratnesh: subtracting CTS duration
                            NS_LOG_DEBUG ("Backoff debug : " << timeSinceBackoffStart << " " <<m_backoffCount << " " << m_slotTime);
                        }
                        else
                        {
                            timeSinceBackoffStart = Simulator::Now () - m_backoffStartTime; 
                        }
                        NS_ASSERT (timeSinceBackoffStart < m_backoffCount * m_slotTime);
                        // Decrement backoff count for every full and fractional m_slot time
                        while (timeSinceBackoffStart > Seconds (0) && m_backoffCount > 0)
                        {
                            m_backoffCount--;
                            timeSinceBackoffStart -= m_slotTime;
                        }
                        m_waitForBusyEventId = Simulator::Schedule (duration, &LbtAccessManager::TransitionFromBusy, this);
                        m_lastBusyTime = Simulator::Now () + duration;
                        NS_LOG_DEBUG ("Suspend backoff, go busy until " << m_lastBusyTime.GetMicroSeconds ());
                        SetLbtState (BUSY);
                    }
                    break;
                default:
                    NS_FATAL_ERROR ("Should be unreachable " << GetLbtState ());
            }
        }

    void
        LbtAccessManager::RequestAccessAfterDefer ()
        {
            NS_LOG_FUNCTION (this);
            // XXX: Ratnesh : Added to handle a corner case
            // if we are in defer, backoff wait should not already be running
            if (m_waitForBackoffEventId.IsRunning ())
            {
                NS_LOG_DEBUG ("Cancelling Backoff");
                m_waitForBackoffEventId.Cancel ();
            }
            // if channel has remained idle so far, wait for a number of backoff
            // slots to see if it is remains idle
            if (GetLbtState () == WAIT_FOR_DEFER)
            {
                if (m_backoffCount == 0)
                {
                    NS_LOG_DEBUG ("Defer succeeded, backoff count already zero");
                    if (m_channelSenseMode == PRECTS || m_channelSenseMode == ENECTS)
                    {
                        NS_LOG_DEBUG ("Scheduling for CTS");
                        m_wifiMacLow->StartLaaCtsToSelfTx(m_txop-m_deferTime- MicroSeconds (45)); // XXX: 45 microseconds for NAV processing
                        m_waitForCtsEventId = Simulator::Schedule ( MicroSeconds (44), &LbtAccessManager::RequestAccessAfterCts, this);
                        SetLbtState (BUSY);
                    }
                    else
                    {
                        SetGrant();
                    }
                }
                else
                {
                    NS_LOG_DEBUG ("Defer succeeded, scheduling for " << m_backoffCount << " backoffSlots");
                    m_backoffStartTime = Simulator::Now ();
                    Time timeForBackoff = m_slotTime * m_backoffCount;
                    m_waitForBackoffEventId = Simulator::Schedule (timeForBackoff, &LbtAccessManager::RequestAccessAfterBackoff, this);
                    SetLbtState (WAIT_FOR_BACKOFF);
                }
            }
            else
            {
                NS_LOG_DEBUG ("Was not deferring, scheduling for " << m_deferTime.GetMicroSeconds () << " microseconds");
                m_waitForDeferEventId = Simulator::Schedule (m_deferTime, &LbtAccessManager::RequestAccessAfterDefer, this);
                SetLbtState (WAIT_FOR_DEFER);
            }
        }

    void
        LbtAccessManager::RequestAccessAfterBackoff ()
        {
            NS_LOG_FUNCTION (this);
            // if the channel has remained idle, grant access now for a TXOP
            if (GetLbtState () == WAIT_FOR_BACKOFF)
            {
                //if (m_useCtsToSelf)
                if (m_channelSenseMode == PRECTS || m_channelSenseMode == ENECTS)
                {
                    // XXX: Ratnesh: waiting for CTS transmission before transmitting the subframes
                    // if (m_txop + m_lastReqStartTime - Simulator::Now () > 0)
                    if (m_txop > 0)
                    {
                        Time ctsEnd = MilliSeconds(int(double((Simulator::Now ()).GetInteger())/1000000)+m_txop.GetInteger()/1000000);
                        NS_LOG_DEBUG ("****Backoff succeeded, scheduling for CTS " << m_lastReqStartTime << " " << ctsEnd);
                        // m_wifiMacLow->StartLaaCtsToSelfTx(m_txop);
                        // m_wifiMacLow->StartLaaCtsToSelfTx(m_txop + m_lastReqStartTime - Simulator::Now ());
                        m_wifiMacLow->StartLaaCtsToSelfTx(ctsEnd-Simulator::Now ()- MicroSeconds (45));
                        m_backoffCount = 0;
                        m_waitForCtsEventId = Simulator::Schedule ( MicroSeconds (44), &LbtAccessManager::RequestAccessAfterCts, this);
                        SetLbtState (BUSY);
                    }
                    else
                    {
                        // This can be handled in one of the two way
                        // 1) Reject the request for being old
                        //
                        NS_LOG_DEBUG ("Request was older than TXOP duration: Rejecting the request " <<m_lastReqStartTime << " " <<((Simulator::Now () - m_lastReqStartTime).GetInteger() % m_txop.GetInteger()));
                        m_backoffCount = 0;
                        SetLbtState (IDLE);

                        // 2) Give grant for next possible txop duration
                        //
                        // NS_LOG_DEBUG ("Backoff succeeded but Request was older than TXOP duration, scheduling for CTS " <<m_lastReqStartTime);
                        // Time temp_t = NanoSeconds (((Simulator::Now () - m_lastReqStartTime).GetInteger() % m_txop.GetInteger()));
                        // m_wifiMacLow->StartLaaCtsToSelfTx(m_txop - temp_t);
                        // m_backoffCount = 0;
                        // m_waitForCtsEventId = Simulator::Schedule ( MicroSeconds (60), &LbtAccessManager::RequestAccessAfterCts, this);
                    }
                }
                else
                {
                    SetGrant();
                    m_backoffCount = 0;
                }
            }
            else
            { 
                NS_FATAL_ERROR ("Unreachable?");
            }
        }
    // XXX: Ratnesh
    void
        LbtAccessManager::RequestAccessAfterCts ()
        {
            NS_LOG_FUNCTION (this);
            SetGrant();
        }

    uint32_t
        LbtAccessManager::GetBackoffSlots ()
        {
            NS_LOG_FUNCTION (this);
            // Integer between 0 and m_cw
            return (m_rng->GetInteger (0, m_cw.Get ()));
        }

    void
        LbtAccessManager::NotifyWakeupNow ()
        {
            NS_LOG_FUNCTION (this);
            //ResetCw ();
        }

    void
        LbtAccessManager::NotifyRxStartNow (Time duration)
        {
            NS_LOG_FUNCTION (this << duration.GetSeconds ());
            if (m_channelSenseMode == PRE || m_channelSenseMode == PRECTS)
                TransitionToBusy (duration);
            // TransitionToBusy (MicroSeconds (1000));
        }

    void
        LbtAccessManager::NotifyRxEndOkNow ()
        {
            NS_LOG_FUNCTION (this);
            // Ignore; may be busy still
        }

    void
        LbtAccessManager::NotifyRxEndErrorNow ()
        {
            NS_LOG_FUNCTION (this);
            // Ignore; may be busy still
        }

    void
        LbtAccessManager::NotifyTxStartNow (Time duration)
        {
            NS_LOG_FUNCTION (this << duration.GetSeconds ());
            // NS_FATAL_ERROR ("Unimplemented");
        }

    void
        LbtAccessManager::NotifyMaybeCcaBusyStartNow (Time duration)
        {
            NS_LOG_FUNCTION (this << duration.GetSeconds ());
            TransitionToBusy (duration);
            // TransitionToBusy (MicroSeconds (1000));
        }

    void
        LbtAccessManager::NotifySwitchingStartNow (Time duration)
        {
            NS_LOG_FUNCTION (this << duration.GetSeconds ());
            NS_FATAL_ERROR ("Unimplemented");
        }

    void
        LbtAccessManager::NotifySleepNow ()
        {
            NS_LOG_FUNCTION (this);
            NS_FATAL_ERROR ("Unimplemented");
        }

    void
        LbtAccessManager::NotifyNavStartNow (Time duration)
        {
            NS_LOG_FUNCTION (this << duration.GetSeconds ());
            // NS_FATAL_ERROR ("Unimplemented");
            // TransitionToBusy (duration);
            // TransitionToBusy (MicroSeconds (300));
        }

    void
        LbtAccessManager::NotifyNavResetNow (Time duration)
        {
            NS_LOG_FUNCTION (this << duration.GetSeconds ());
            //NS_FATAL_ERROR ("Unimplemented");
        }

    void
        LbtAccessManager::NotifyAckTimeoutStartNow (Time duration)
        {
            NS_LOG_FUNCTION (this << duration.GetSeconds ());
            //NS_FATAL_ERROR ("Unimplemented");
        }

    void
        LbtAccessManager::NotifyAckTimeoutResetNow ()
        {
            NS_LOG_FUNCTION (this);
            //NS_FATAL_ERROR ("Unimplemented");
        }

    void
        LbtAccessManager::NotifyCtsTimeoutStartNow (Time duration)
        {
            NS_LOG_FUNCTION (this << duration.GetSeconds ());
            // NS_FATAL_ERROR ("Unimplemented");
            //TransitionToBusy (duration);
        }

    void
        LbtAccessManager::NotifyCtsTimeoutResetNow ()
        {
            NS_LOG_FUNCTION (this);
            //NS_FATAL_ERROR ("Unimplemented");
        }

    void
        LbtAccessManager::ResetCw (void)
        {
            NS_LOG_FUNCTION (this);
            NS_LOG_DEBUG ("CW reset from " << m_cw  << " to " << m_cwMin);
            m_cw = m_cwMin;
        }

    void
        LbtAccessManager::UpdateFailedCw (void)
        {
            NS_LOG_FUNCTION (this);
            uint32_t oldValue = m_cw.Get ();
            // m_cw = std::min ( 2 * (m_cw.Get () + 1) - 1, m_cwMax);
            m_cw = std::min ( m_cwFactor * (m_cw.Get () + 1) - 1, m_cwMax);
            NS_LOG_DEBUG ("CW updated from " << oldValue << " to " << m_cw);
            m_lastCWUpdateTime = Simulator::Now ();
        }

    void LbtAccessManager::SetGrant()
    {
        NS_LOG_DEBUG ("Granting access through ChannelAccessManager at time " << Simulator::Now ().GetMicroSeconds ());
        ChannelAccessManager::SetGrantDuration (m_txop);
        ChannelAccessManager::DoRequestAccess ();
        SetLbtState (TXOP_GRANTED);
        m_grantRequested = false;

        // save only lates txops - the ones for which we are expecting harq feedback, txops that started at after Simulator::Now() - m_harqFeedbackDelay
        std::vector<Time>::iterator it = m_txopHistory.begin();
        while (it != m_txopHistory.end())
        {
            if (Simulator::Now() - m_harqFeedbackExpirationTime > *it)
            {
                m_txopHistory.erase(it);
            }
            else
            {
                break; // there are no more elements to delete
            }
        }
        m_txopHistory.push_back(GetLastTxopStartTime());
    }

    void
        LbtAccessManager::UpdateCwBasedOnHarq (std::vector<DlInfoListElement_s> m_dlInfoListReceived)
        {
            NS_LOG_FUNCTION (this);
            uint32_t nackCounter = 0;
            bool considerThisHarqFeedback = false;
            NS_LOG_INFO("Update cw at:"<<Simulator::Now().GetMilliSeconds()<<"ms. Txop history size:"<<m_txopHistory.size());

            // Check if this harq feedback should be processed. Rule adopted here is to update CW based on first available harq feedback for the last packet burst. Other feedbacks of the same burst ignore.
            std::vector<Time>::iterator it = m_txopHistory.begin();
            while (it!=m_txopHistory.end())
            {
                Time oldestTxop = *it;
                // check if harq feedback belongs to oldest txop
                if (Simulator::Now() - m_harqFeedbackDelay  >= oldestTxop)
                {
                    considerThisHarqFeedback = true;
                    m_txopHistory.erase(it); // ignore other feedbacks for this txop
                    NS_LOG_INFO("The feedback maybe belongs to the oldest txop. Delete the oldest and check the next oldest. ");
                }
                else
                { // if we didn't find txop in the list to which corresponds this harq feedback then ignore it
                    if (!considerThisHarqFeedback)
                    {
                        considerThisHarqFeedback = false;
                        NS_LOG_INFO("Ignore this feedback. The oldest txop for which we can accept feedback is:"<<oldestTxop.GetMilliSeconds());
                    }
                    else
                    {
                        NS_LOG_INFO("Use this feedback. The feedback belongs to the last erased txop. We erased it in order to ignore other feedbacks for that txop.");
                    }
                    break;
                }
            }

            if (!considerThisHarqFeedback)
            {
                NS_LOG_INFO("Feedback ignored at:"<<Simulator::Now());
                return;
            }
            else
            {
                NS_LOG_INFO("Feedback considered at:"<<Simulator::Now());
            }

            // update harq feedback trace source
            NS_LOG_INFO("dlInfoListReceived size : " << m_dlInfoListReceived.size());
            std::vector<uint8_t> harqFeedback;
            for (uint16_t i = 0; i < m_dlInfoListReceived.size(); i++)
            {
                NS_LOG_INFO("dlInfoListReceived id : " << m_dlInfoListReceived.at(i).m_harqProcessId);
                for (uint8_t layer = 0; layer < m_dlInfoListReceived.at(i).m_harqStatus.size (); layer++)
                {
                    if (m_dlInfoListReceived.at(i).m_harqStatus.at(layer) == DlInfoListElement_s::ACK)
                    {
                        harqFeedback.push_back(0);
                    }
                    else if (m_dlInfoListReceived.at(i).m_harqStatus.at(layer) == DlInfoListElement_s::NACK)
                    {
                        harqFeedback.push_back(1);
                        nackCounter++;
                    }
                }
            }

            if (harqFeedback.size () == 0)
            {
                NS_LOG_INFO("Harq feedback empty at:"<<Simulator::Now());
                return;
            }

            bool updateFailedCw = false;

            switch (m_cwUpdateRule)
            {
                case ALL_NACKS:
                    {
                        if (double(nackCounter)/(double)harqFeedback.size() == 1)
                        {
                            updateFailedCw = true;
                        }
                    }
                    break;
                case ANY_NACK:
                    {
                        if (nackCounter > 0)
                        {
                            updateFailedCw  = true;
                        }
                    }
                    break;
                case NACKS_80_PERCENT:
                    {
                        if (double(nackCounter)/(double)harqFeedback.size() >= 0.8)
                        {
                            updateFailedCw = true;
                        }
                    }
                    break;
                case NACKS_10_PERCENT:
                    {
                        if (double(nackCounter)/(double)harqFeedback.size() >= 0.1)
                        {
                            updateFailedCw = true;
                        }
                    }
                    break;
                default:
                    {
                        NS_FATAL_ERROR ("Unreachable?");
                        return;
                    }
            }

            if (updateFailedCw)
            {
                UpdateFailedCw ();
            }
            else
            {
                ResetCw ();
            }
        }

    uint32_t
        LbtAccessManager::GetCurrentBackoffCount (void) const
        {
            NS_LOG_FUNCTION (this);
            return m_backoffCount;
        }

} // ns3 namespace

