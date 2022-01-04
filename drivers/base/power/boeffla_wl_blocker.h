/*
 * Author: andip71, 01.09.2017
 * Enhanted by xNombre (Andrzej Perczak)
 *
 * Version 1.2.0
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define BOEFFLA_WL_BLOCKER_VERSION	"1.2.0"

#define LIST_WL_DEFAULT		 "wlan;wlan_wow_wl;wlan_extscan_wl;netmgr_wl;NETLINK;IPA_WS;wlan_ipa;wlan_pno_wl;wcnss_filter_lock;[timerfd];IPA_CLIENT_APPS_LAN_CONS;IPA_CLIENT_APPS_WAN_CONS;rmnet_ipa%d;IPCRTR_lpass_rx;hal_bluetooth_lock;DIAG_WS;qcom_sap_wakelock;sensor_ind;wlan_rx_wake;wlan_ctrl_wake;wlan_wake;qbt_wake_source;998000.qcom,qup_uart"
#define LENGTH_LIST_WL_DEFAULT	 sizeof(LIST_WL_DEFAULT)
#define LENGTH_LIST_WL		 255
#define LENGTH_LIST_WL_SEARCH	 LENGTH_LIST_WL + LENGTH_LIST_WL_DEFAULT + 5

extern void wakeup_source_deactivate(struct wakeup_source *ws);

#ifdef CONFIG_BOEFFLA_WL_BLOCKER
bool is_blocked(struct wakeup_source *ws);
#else
inline bool is_blocked(struct wakeup_source *ws)
{
	return 0;
}
#endif
