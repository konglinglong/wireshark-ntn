/* packet-nr-rrc-ntn-template.h
 * Copyright 2018-2026, Pascal Quantin
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PACKET_NR_RRC_NTN_H
#define PACKET_NR_RRC_NTN_H

#include "packet-nr-rrc-ntn-exp.h"
int dissect_nr_rrc_ntn_nr_RLF_Report_r16_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_nr_rrc_ntn_subCarrierSpacingCommon_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
int dissect_nr_rrc_ntn_rach_ConfigCommonIAB_r16_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_, void *data _U_);
#endif  /* PACKET_NR_RRC_NTN_H */
