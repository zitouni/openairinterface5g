/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file f1ap_cu_ue_context_management.c
 * \brief F1AP UE Context Management, CU side
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#include "f1ap_common.h"
#include "f1ap_encoder.h"
#include "f1ap_itti_messaging.h"
#include "f1ap_cu_ue_context_management.h"
#include <string.h>

#include "rrc_extern.h"
#include "openair2/RRC/NR/rrc_gNB_NGAP.h"

static void f1ap_write_drb_qos_param(const f1ap_qos_flow_level_qos_parameters_t *drb_qos_in, F1AP_QoSFlowLevelQoSParameters_t *asn1_qosparam)
{
  int type = drb_qos_in->qos_characteristics.qos_type;

  const f1ap_qos_characteristics_t *drb_qos_char_in = &drb_qos_in->qos_characteristics;
  if (type == non_dynamic) {
    asn1_qosparam->qoS_Characteristics.present = F1AP_QoS_Characteristics_PR_non_Dynamic_5QI;
    asn1cCalloc(asn1_qosparam->qoS_Characteristics.choice.non_Dynamic_5QI, tmp);

    /* 5QI */
    tmp->fiveQI = drb_qos_char_in->non_dynamic.fiveqi;
  } else {
    asn1_qosparam->qoS_Characteristics.present = F1AP_QoS_Characteristics_PR_dynamic_5QI;
    asn1cCalloc(asn1_qosparam->qoS_Characteristics.choice.dynamic_5QI, tmp);
    /* qoSPriorityLevel */
    tmp->qoSPriorityLevel = drb_qos_char_in->dynamic.qos_priority_level;
    /* packetDelayBudget */
    tmp->packetDelayBudget = drb_qos_char_in->dynamic.packet_delay_budget;
    /* packetErrorRate */
    tmp->packetErrorRate.pER_Scalar = drb_qos_char_in->dynamic.packet_error_rate.per_scalar;
    tmp->packetErrorRate.pER_Exponent = drb_qos_char_in->dynamic.packet_error_rate.per_scalar;

    /* OPTIONAL delayCritical */
    // asn1cCallocOne(asn1_qosparam->qoS_Characteristics.choice.dynamic_5QI->delayCritical, 1L);

    /* OPTIONAL averagingWindow */
    // asn1cCallocOne(asn1_qosparam->qoS_Characteristics.choice.dynamic_5QI->averagingWindow, 1L);

    /* OPTIONAL maxDataBurstVolume */
    // asn1cCallocOne(asn1_qosparam->qoS_Characteristics.choice.dynamic_5QI->maxDataBurstVolume, 1L);
  }

  {
    asn1_qosparam->nGRANallocationRetentionPriority.priorityLevel = drb_qos_in->alloc_reten_priority.priority_level;
    asn1_qosparam->nGRANallocationRetentionPriority.pre_emptionCapability = drb_qos_in->alloc_reten_priority.preemption_capability;
    asn1_qosparam->nGRANallocationRetentionPriority.pre_emptionVulnerability =
        drb_qos_in->alloc_reten_priority.preemption_vulnerability;
  } // nGRANallocationRetentionPriority

  /* OPTIONAL */
  /* gBR_QoS_Flow_Information */
  if (0) {
    asn1cCalloc(asn1_qosparam->gBR_QoS_Flow_Information, tmp);
    asn_long2INTEGER(&tmp->maxFlowBitRateDownlink, 1L);
    asn_long2INTEGER(&tmp->maxFlowBitRateUplink, 1L);
    asn_long2INTEGER(&tmp->guaranteedFlowBitRateDownlink, 1L);
    asn_long2INTEGER(&tmp->guaranteedFlowBitRateUplink, 1L);

    /* OPTIONAL */
    /* maxPacketLossRateDownlink */
    // asn1cCallocOne(asn1_qosparam->gBR_QoS_Flow_Information->maxPacketLossRateDownlink, 1L);

    /* OPTIONAL */
    /* maxPacketLossRateUplink */
    //asn1cCallocOne(asn1_qosparam->gBR_QoS_Flow_Information->maxPacketLossRateUplink, 1L);
  }

  /* OPTIONAL */
  /* reflective_QoS_Attribute */
  if (0) {
    asn1cCallocOne(asn1_qosparam->reflective_QoS_Attribute, 1L);
  }
}

static void f1ap_write_drb_nssai(const nssai_t *nssai, F1AP_SNSSAI_t *asn1_nssai)
{
  OCTET_STRING_fromBuf(&asn1_nssai->sST, (char *)&nssai->sst, 1);

  /* OPTIONAL */
  if (nssai->sd != 0xffffff)
    OCTET_STRING_fromBuf(asn1_nssai->sD, (char *)&nssai->sd, 3);
}

static void f1ap_write_flows_mapped(const f1ap_flows_mapped_to_drb_t *flows_mapped, F1AP_Flows_Mapped_To_DRB_List_t *asn1_flows_mapped, int n)
{
  for (int k = 0; k < n; k++) {
    asn1cSequenceAdd(asn1_flows_mapped->list, F1AP_Flows_Mapped_To_DRB_Item_t, flow_item);

    const f1ap_flows_mapped_to_drb_t *qos_flow_in = flows_mapped + k;

    /* qoSFlowIndicator */
    flow_item->qoSFlowIdentifier = qos_flow_in->qfi;

    /* qoSFlowLevelQoSParameters */
    const f1ap_qos_flow_level_qos_parameters_t *flow_qos_params_in = &qos_flow_in->qos_params;
    /* qoS_Characteristics */

    F1AP_QoS_Characteristics_t *QosParams = &flow_item->qoSFlowLevelQoSParameters.qoS_Characteristics;
    const f1ap_qos_characteristics_t *flow_qos_char_in = &flow_qos_params_in->qos_characteristics;

    int type = flow_qos_params_in->qos_characteristics.qos_type;
    if (type == non_dynamic) {
      QosParams->present = F1AP_QoS_Characteristics_PR_non_Dynamic_5QI;
      asn1cCalloc(QosParams->choice.non_Dynamic_5QI, tmp);

      /* 5QI */
      tmp->fiveQI = flow_qos_char_in->non_dynamic.fiveqi;
    } else {
      QosParams->present = F1AP_QoS_Characteristics_PR_dynamic_5QI;
      asn1cCalloc(QosParams->choice.dynamic_5QI, tmp);
      /* qoSPriorityLevel */
      tmp->qoSPriorityLevel = flow_qos_char_in->dynamic.qos_priority_level;
      /* packetDelayBudget */
      tmp->packetDelayBudget = flow_qos_char_in->dynamic.packet_delay_budget;
      /* packetErrorRate */
      tmp->packetErrorRate.pER_Scalar = flow_qos_char_in->dynamic.packet_error_rate.per_scalar;
      tmp->packetErrorRate.pER_Exponent = flow_qos_char_in->dynamic.packet_error_rate.per_exponent;

      /* OPTIONAL delayCritical */
      //asn1cCallocOne(QosParams->choice.dynamic_5QI->delayCritical, 1);

      /* OPTIONAL averagingWindow */
      //asn1cCallocOne(QosParams->choice.dynamic_5QI->averagingWindow, 1);

      /* OPTIONAL maxDataBurstVolume */
      //asn1cCallocOne(QosParams->choice.dynamic_5QI->maxDataBurstVolume, 1);
    }

    /* nGRANallocationRetentionPriority */
    {
      flow_item->qoSFlowLevelQoSParameters.nGRANallocationRetentionPriority.priorityLevel =
          flow_qos_params_in->alloc_reten_priority.priority_level;
      flow_item->qoSFlowLevelQoSParameters.nGRANallocationRetentionPriority.pre_emptionCapability =
          flow_qos_params_in->alloc_reten_priority.preemption_capability;
      flow_item->qoSFlowLevelQoSParameters.nGRANallocationRetentionPriority.pre_emptionVulnerability =
          flow_qos_params_in->alloc_reten_priority.preemption_vulnerability;
    } // nGRANallocationRetentionPriority

    /* OPTIONAL */
    /* gBR_QoS_Flow_Information */
    if (0) {
      asn1cCalloc(flow_item->qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information, tmp);
      asn_long2INTEGER(&tmp->maxFlowBitRateDownlink, 1L);
      asn_long2INTEGER(&tmp->maxFlowBitRateUplink, 1L);
      asn_long2INTEGER(&tmp->guaranteedFlowBitRateDownlink, 1L);
      asn_long2INTEGER(&tmp->guaranteedFlowBitRateUplink, 1L);

      /* OPTIONAL maxPacketLossRateDownlink */
      //asn1cCallocOne(flows_mapped_to_drb_item->qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information->maxPacketLossRateDownlink, 1L);

      /* OPTIONAL maxPacketLossRateUplink */
      //asn1cCallocOne(flows_mapped_to_drb_item->qoSFlowLevelQoSParameters.gBR_QoS_Flow_Information->maxPacketLossRateUplink, 1L);
    }

    /* OPTIONAL reflective_QoS_Attribute */
    //asn1cCallocOne(flows_mapped_to_drb_item->qoSFlowLevelQoSParameters.reflective_QoS_Attribute, 1L);
  }
}

int CU_send_UE_CONTEXT_SETUP_REQUEST(sctp_assoc_t assoc_id, f1ap_ue_context_setup_t *f1ap_ue_context_setup_req)
{
  F1AP_F1AP_PDU_t  pdu= {0};
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextSetup;
  tmp->criticality   = F1AP_Criticality_reject;
  tmp->value.present = F1AP_InitiatingMessage__value_PR_UEContextSetupRequest;
  F1AP_UEContextSetupRequest_t    *out = &pdu.choice.initiatingMessage->value.choice.UEContextSetupRequest;
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = f1ap_ue_context_setup_req->gNB_CU_ue_id;

  /* optional */
  /* c2. GNB_DU_UE_F1AP_ID */
  if (f1ap_ue_context_setup_req->gNB_DU_ue_id) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie2);
    ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
    ie2->criticality                    = F1AP_Criticality_ignore;
    ie2->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_GNB_DU_UE_F1AP_ID;
    ie2->value.choice.GNB_DU_UE_F1AP_ID = f1ap_ue_context_setup_req->gNB_DU_ue_id;
  }

  /* mandatory */
  /* c3. SpCell_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie3);
  ie3->id                             = F1AP_ProtocolIE_ID_id_SpCell_ID;
  ie3->criticality                    = F1AP_Criticality_reject;
  ie3->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_NRCGI;
  /* - nRCGI */
  addnRCGI(ie3->value.choice.NRCGI, f1ap_ue_context_setup_req);
  /* mandatory */
  /* c4. ServCellIndex */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie4);
  ie4->id                             = F1AP_ProtocolIE_ID_id_ServCellIndex;
  ie4->criticality                    = F1AP_Criticality_reject;
  ie4->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_ServCellIndex;
  ie4->value.choice.ServCellIndex = 0;

  /* optional */
  /* c5. CellULConfigured */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie5);
    ie5->id                             = F1AP_ProtocolIE_ID_id_SpCellULConfigured;
    ie5->criticality                    = F1AP_Criticality_ignore;
    ie5->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_CellULConfigured;
    ie5->value.choice.CellULConfigured = F1AP_CellULConfigured_ul;
  }

  /* mandatory */
  /* c6. CUtoDURRCInformation */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie6);
  ie6->id                             = F1AP_ProtocolIE_ID_id_CUtoDURRCInformation;
  ie6->criticality                    = F1AP_Criticality_reject;
  ie6->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_CUtoDURRCInformation;

  if (f1ap_ue_context_setup_req->cu_to_du_rrc_information!=NULL) {
    /* optional */
    /* 6.1 cG_ConfigInfo */
    if(f1ap_ue_context_setup_req->cu_to_du_rrc_information->cG_ConfigInfo!=NULL){
      asn1cCalloc(ie6->value.choice.CUtoDURRCInformation.cG_ConfigInfo, cG_ConfigInfo);
      OCTET_STRING_fromBuf(cG_ConfigInfo, (const char *)f1ap_ue_context_setup_req->cu_to_du_rrc_information->cG_ConfigInfo,
        f1ap_ue_context_setup_req->cu_to_du_rrc_information->cG_ConfigInfo_length);
    }
    /* optional */
    /* 6.2 uE_CapabilityRAT_ContainerList */
    if(f1ap_ue_context_setup_req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList!=NULL){
      asn1cCalloc(ie6->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList, uE_CapabilityRAT_ContainerList );
      OCTET_STRING_fromBuf(uE_CapabilityRAT_ContainerList, (const char *)f1ap_ue_context_setup_req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList,
        f1ap_ue_context_setup_req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList_length);
    }
    /* optional */
    /* 6.3 measConfig */
    if(f1ap_ue_context_setup_req->cu_to_du_rrc_information->measConfig!=NULL){
      asn1cCalloc(ie6->value.choice.CUtoDURRCInformation.measConfig,  measConfig);
      OCTET_STRING_fromBuf(measConfig, (const char*)f1ap_ue_context_setup_req->cu_to_du_rrc_information->measConfig,
        f1ap_ue_context_setup_req->cu_to_du_rrc_information->measConfig_length);
    }

    /* optional */
    /* HandoverPreparationInformation */
    if (f1ap_ue_context_setup_req->cu_to_du_rrc_information->handoverPreparationInfo_length > 0) {
      DevAssert(f1ap_ue_context_setup_req->cu_to_du_rrc_information->handoverPreparationInfo != NULL);
      F1AP_ProtocolExtensionContainer_10696P60_t *p = calloc(1, sizeof(*p));
      ie6->value.choice.CUtoDURRCInformation.iE_Extensions = (struct F1AP_ProtocolExtensionContainer *)p;
      asn1cSequenceAdd(p->list, F1AP_CUtoDURRCInformation_ExtIEs_t, ie_ext);
      ie_ext->id = F1AP_ProtocolIE_ID_id_HandoverPreparationInformation;
      ie_ext->criticality = F1AP_Criticality_ignore;
      ie_ext->extensionValue.present = F1AP_CUtoDURRCInformation_ExtIEs__extensionValue_PR_HandoverPreparationInformation;
      OCTET_STRING_fromBuf(&ie_ext->extensionValue.choice.HandoverPreparationInformation,
                           (const char *)f1ap_ue_context_setup_req->cu_to_du_rrc_information->handoverPreparationInfo,
                           f1ap_ue_context_setup_req->cu_to_du_rrc_information->handoverPreparationInfo_length);
    }
  }

  /* optional */
  if (0) {
    /* c7. Candidate_SpCell_List */
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie7);
    ie7->id = F1AP_ProtocolIE_ID_id_Candidate_SpCell_List; // 90
    ie7->criticality = F1AP_Criticality_ignore;
    ie7->value.present = F1AP_UEContextSetupRequestIEs__value_PR_Candidate_SpCell_List;

    for (int i = 0; i < 1; i++) {
      asn1cSequenceAdd(ie7->value.choice.Candidate_SpCell_List.list, F1AP_Candidate_SpCell_ItemIEs_t, candidate_spCell_item_ies);
      candidate_spCell_item_ies->id = F1AP_ProtocolIE_ID_id_Candidate_SpCell_Item; // 91
      candidate_spCell_item_ies->criticality = F1AP_Criticality_ignore;
      candidate_spCell_item_ies->value.present = F1AP_Candidate_SpCell_ItemIEs__value_PR_Candidate_SpCell_Item;
      /* 7.1 Candidate_SpCell_Item */
      F1AP_Candidate_SpCell_Item_t *candidate_spCell_item = &candidate_spCell_item_ies->value.choice.Candidate_SpCell_Item;
      /* - candidate_SpCell_ID */
      // FixMe: first cell ???
      addnRCGI(candidate_spCell_item->candidate_SpCell_ID, f1ap_ue_context_setup_req);
      /* TODO add correct mcc/mnc */
    }
  }

  /* optional */
  /* c8. DRXCycle */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie8);
    ie8->id                             = F1AP_ProtocolIE_ID_id_DRXCycle;
    ie8->criticality                    = F1AP_Criticality_ignore;
    ie8->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_DRXCycle;
    /* 8.1 longDRXCycleLength */
    ie8->value.choice.DRXCycle.longDRXCycleLength = F1AP_LongDRXCycleLength_ms10; // enum

    /* optional */
    /* 8.2 shortDRXCycleLength */
    if (0) {
      asn1cCallocOne(ie6->value.choice.DRXCycle.shortDRXCycleLength,
                     F1AP_ShortDRXCycleLength_ms2); // enum
    }

    /* optional */
    /* 8.3 shortDRXCycleTimer */
    if (0) {
      asn1cCallocOne(ie8->value.choice.DRXCycle.shortDRXCycleTimer,
                     123L);
    }
  }

  /* optional */
  /* c9. ResourceCoordinationTransferContainer */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie9);
    ie9->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie9->criticality                    = F1AP_Criticality_ignore;
    ie9->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_ResourceCoordinationTransferContainer;
    ie9->value.choice.ResourceCoordinationTransferContainer.buf = malloc(4);
    ie9->value.choice.ResourceCoordinationTransferContainer.size = 4;
    strncpy((char *)ie9->value.choice.ResourceCoordinationTransferContainer.buf, "123", 4);
    OCTET_STRING_fromBuf(&ie9->value.choice.ResourceCoordinationTransferContainer, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
  }

  /* optional */
  if (0) {
    /* c10. SCell_ToBeSetup_List */
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie10);
    ie10->id = F1AP_ProtocolIE_ID_id_SCell_ToBeSetup_List;
    ie10->criticality = F1AP_Criticality_ignore;
    ie10->value.present = F1AP_UEContextSetupRequestIEs__value_PR_SCell_ToBeSetup_List;

    for (int i = 0; i < 1; i++) {
      asn1cSequenceAdd(ie10->value.choice.SCell_ToBeSetup_List.list, F1AP_SCell_ToBeSetup_ItemIEs_t, scell_toBeSetup_item_ies);
      scell_toBeSetup_item_ies->id = F1AP_ProtocolIE_ID_id_SCell_ToBeSetup_Item; // 53
      scell_toBeSetup_item_ies->criticality = F1AP_Criticality_ignore;
      scell_toBeSetup_item_ies->value.present = F1AP_SCell_ToBeSetup_ItemIEs__value_PR_SCell_ToBeSetup_Item;
      /* 10.1 SCell_ToBeSetup_Item */
      F1AP_SCell_ToBeSetup_Item_t *scell_toBeSetup_item = &scell_toBeSetup_item_ies->value.choice.SCell_ToBeSetup_Item;
      /* 10.1.1 sCell_ID */
      addnRCGI(scell_toBeSetup_item->sCell_ID, f1ap_ue_context_setup_req);
      /* TODO correct MCC/MNC */
      /* 10.1.2 sCellIndex */
      scell_toBeSetup_item->sCellIndex = 3; // issue here

      /* OPTIONAL */
      /* 10.1.3 sCellULConfigured*/
      if (0) {
        asn1cCallocOne(scell_toBeSetup_item->sCellULConfigured,
                       F1AP_CellULConfigured_ul_and_sul); // enum
      }
    }
  }

  /* mandatory */
  /* c11. SRBs_ToBeSetup_List */
  if(f1ap_ue_context_setup_req->srbs_to_be_setup_length > 0){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie11);
    ie11->id                             = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetup_List;
    ie11->criticality                    = F1AP_Criticality_reject;  // ?
    ie11->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_SRBs_ToBeSetup_List;

    for (int i=0; i<f1ap_ue_context_setup_req->srbs_to_be_setup_length; i++) {
      asn1cSequenceAdd(ie11->value.choice.SRBs_ToBeSetup_List.list, F1AP_SRBs_ToBeSetup_ItemIEs_t, srbs_toBeSetup_item_ies);
      srbs_toBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetup_Item; // 73
      srbs_toBeSetup_item_ies->criticality = F1AP_Criticality_reject;
      srbs_toBeSetup_item_ies->value.present = F1AP_SRBs_ToBeSetup_ItemIEs__value_PR_SRBs_ToBeSetup_Item;
      /* 11.1 SRBs_ToBeSetup_Item */
      F1AP_SRBs_ToBeSetup_Item_t *srbs_toBeSetup_item=&srbs_toBeSetup_item_ies->value.choice.SRBs_ToBeSetup_Item;
      /* 11.1.1 sRBID */
      srbs_toBeSetup_item->sRBID = f1ap_ue_context_setup_req->srbs_to_be_setup[i].srb_id;
      /* OPTIONAL */
      /* 11.1.2 duplicationIndication */
      asn1cCallocOne(srbs_toBeSetup_item->duplicationIndication,
                   F1AP_DuplicationIndication_true); // enum
    }
  }

  /* mandatory */
  /* c12. DRBs_ToBeSetup_List */
  if(f1ap_ue_context_setup_req->drbs_to_be_setup_length){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie12);
    ie12->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetup_List;
    ie12->criticality                    = F1AP_Criticality_reject;
    ie12->value.present = F1AP_UEContextSetupRequestIEs__value_PR_DRBs_ToBeSetup_List;

    for (int i = 0; i < f1ap_ue_context_setup_req->drbs_to_be_setup_length; i++) {
      const f1ap_drb_to_be_setup_t *drb = &f1ap_ue_context_setup_req->drbs_to_be_setup[i];
      asn1cSequenceAdd(ie12->value.choice.DRBs_ToBeSetup_List.list, F1AP_DRBs_ToBeSetup_ItemIEs_t, drbs_toBeSetup_item_ies);
      drbs_toBeSetup_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetup_Item;
      drbs_toBeSetup_item_ies->criticality   = F1AP_Criticality_reject;
      drbs_toBeSetup_item_ies->value.present = F1AP_DRBs_ToBeSetup_ItemIEs__value_PR_DRBs_ToBeSetup_Item;
      /* 12.1 DRBs_ToBeSetup_Item */
      F1AP_DRBs_ToBeSetup_Item_t *drbs_toBeSetup_item=&drbs_toBeSetup_item_ies->value.choice.DRBs_ToBeSetup_Item;
      /* 12.1.1 dRBID */
      drbs_toBeSetup_item->dRBID = drb->drb_id;
      /* 12.1.2 qoSInformation */
      int some_decide_qos = 0; // BK: Need Check

      if (some_decide_qos) {
        drbs_toBeSetup_item->qoSInformation.present = F1AP_QoSInformation_PR_eUTRANQoS;
        /*  12.1.2.1 eUTRANQoS */
        asn1cCalloc(drbs_toBeSetup_item->qoSInformation.choice.eUTRANQoS, eUTRANQoS);
        /*  12.1.2.1.1 qCI */
        eUTRANQoS->qCI = 254L;
        /*  12.1.2.1.2 allocationAndRetentionPriority */
        {
          /*  12.1.2.1.2.1 priorityLevel */
          eUTRANQoS->allocationAndRetentionPriority.priorityLevel = F1AP_PriorityLevel_highest; // enum
          /*  12.1.2.1.2.2 pre_emptionCapability */
          eUTRANQoS->allocationAndRetentionPriority.pre_emptionCapability = F1AP_Pre_emptionCapability_may_trigger_pre_emption; // enum
          /*  12.1.2.1.2.2 pre_emptionVulnerability */
          eUTRANQoS->allocationAndRetentionPriority.pre_emptionVulnerability = F1AP_Pre_emptionVulnerability_not_pre_emptable; // enum
        }

        /* OPTIONAL */
        /*  12.1.2.1.3 gbrQosInformation */
        if (0) {
          eUTRANQoS->gbrQosInformation = (F1AP_GBR_QosInformation_t *)calloc(1, sizeof(F1AP_GBR_QosInformation_t));
          asn_long2INTEGER(&eUTRANQoS->gbrQosInformation->e_RAB_MaximumBitrateDL, 1L);
          asn_long2INTEGER(&eUTRANQoS->gbrQosInformation->e_RAB_MaximumBitrateUL, 1L);
          asn_long2INTEGER(&eUTRANQoS->gbrQosInformation->e_RAB_GuaranteedBitrateDL, 1L);
          asn_long2INTEGER(&eUTRANQoS->gbrQosInformation->e_RAB_GuaranteedBitrateUL, 1L);
        }
      } else {
        /* 12.1.2 DRB_Information */
        const f1ap_drb_information_t *drb_info = &drb->drb_info;
        drbs_toBeSetup_item->qoSInformation.present = F1AP_QoSInformation_PR_choice_extension;
        F1AP_QoSInformation_ExtIEs_t *ie = (F1AP_QoSInformation_ExtIEs_t *)calloc(1, sizeof(*ie));
        ie->id                             = F1AP_ProtocolIE_ID_id_DRB_Information;
        ie->criticality                    = F1AP_Criticality_reject;
        ie->value.present                  = F1AP_QoSInformation_ExtIEs__value_PR_DRB_Information;
        F1AP_DRB_Information_t   *DRB_Information = &ie->value.choice.DRB_Information;
        drbs_toBeSetup_item->qoSInformation.choice.choice_extension = (struct F1AP_ProtocolIE_SingleContainer *)ie;

        /* 12.1.2.1 dRB_QoS */
        f1ap_write_drb_qos_param(&drb_info->drb_qos, &DRB_Information->dRB_QoS);

        /* 12.1.2.2 sNSSAI */
        f1ap_write_drb_nssai(&drb->nssai, &DRB_Information->sNSSAI);

        /* OPTIONAL */
        /* 12.1.2.3 notificationControl */
        if (0) {
          asn1cCallocOne(DRB_Information->notificationControl,
            F1AP_NotificationControl_active); // enum
        }

        /* 12.1.2.4 flows_Mapped_To_DRB_List */  // BK: need verifiy
        f1ap_write_flows_mapped(drb_info->flows_mapped_to_drb, &DRB_Information->flows_Mapped_To_DRB_List, drb_info->flows_to_be_setup_length);
      } // if some_decide_qos

      /* 12.1.3 uLUPTNLInformation_ToBeSetup_List */
      for (int j = 0; j < f1ap_ue_context_setup_req->drbs_to_be_setup[i].up_ul_tnl_length; j++) {
        DevAssert(f1ap_ue_context_setup_req->drbs_to_be_setup[i].up_ul_tnl[j].teid > 0);
        /*  12.3.1 ULTunnels_ToBeSetup_Item */
        asn1cSequenceAdd(drbs_toBeSetup_item->uLUPTNLInformation_ToBeSetup_List.list,
          F1AP_ULUPTNLInformation_ToBeSetup_Item_t, uLUPTNLInformation_ToBeSetup_Item);
        uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
        asn1cCalloc( uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.choice.gTPTunnel,
          gTPTunnel);
        /* 12.3.1.1.1 transportLayerAddress */
        TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(f1ap_ue_context_setup_req->drbs_to_be_setup[i].up_ul_tnl[j].tl_address,
          &gTPTunnel->transportLayerAddress);
        /* 12.3.1.1.2 gTP_TEID */
        INT32_TO_OCTET_STRING(f1ap_ue_context_setup_req->drbs_to_be_setup[i].up_ul_tnl[j].teid,
          &gTPTunnel->gTP_TEID);
      }

      /* 12.1.4 rLCMode */
      /* TODO use rlc_mode from f1ap_drb_to_be_setup */
      switch (f1ap_ue_context_setup_req->drbs_to_be_setup[i].rlc_mode) {
        case RLC_MODE_AM:
          drbs_toBeSetup_item->rLCMode = F1AP_RLCMode_rlc_am;
          break;

        default:
          drbs_toBeSetup_item->rLCMode = F1AP_RLCMode_rlc_um_bidirectional;
      }

      /* OPTIONAL */
      /* 12.1.5 ULConfiguration */
      if (0) {
        asn1cCalloc(drbs_toBeSetup_item->uLConfiguration, tmp);
        tmp->uLUEConfiguration = F1AP_ULUEConfiguration_no_data;
      }

      /* OPTIONAL */
      /* 12.1.6 duplicationActivation */
      if (0) {
        asn1cCalloc(drbs_toBeSetup_item->duplicationActivation, tmp);
        *tmp = F1AP_DuplicationActivation_active;  // enum
      }
    }
  }

  /* OPTIONAL */
  /* InactivityMonitoringRequest */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie12);
    ie12->id                             = F1AP_ProtocolIE_ID_id_InactivityMonitoringRequest;
    ie12->criticality                    = F1AP_Criticality_reject;
    ie12->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_InactivityMonitoringRequest;
    ie12->value.choice.InactivityMonitoringRequest = F1AP_InactivityMonitoringRequest_true; // 0
  }

  /* OPTIONAL */
  /* RAT_FrequencyPriorityInformation */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie13);
    ie13->id                             = F1AP_ProtocolIE_ID_id_RAT_FrequencyPriorityInformation;
    ie13->criticality                    = F1AP_Criticality_reject;
    ie13->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_RAT_FrequencyPriorityInformation;
    int endc = 1; // RK: Get this from somewhere ...

    if (endc) {
      ie13->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_eNDC;
      ie13->value.choice.RAT_FrequencyPriorityInformation.choice.eNDC = 11L;
    } else {
      ie13->value.choice.RAT_FrequencyPriorityInformation.present = F1AP_RAT_FrequencyPriorityInformation_PR_nGRAN;
      ie13->value.choice.RAT_FrequencyPriorityInformation.choice.nGRAN = 11L;
    }
  }

  /* OPTIONAL */
  /* RRCContainer */
  if(f1ap_ue_context_setup_req->rrc_container_length > 0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie14);
    ie14->id                             = F1AP_ProtocolIE_ID_id_RRCContainer;
    ie14->criticality = F1AP_Criticality_ignore;
    ie14->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_RRCContainer;
    OCTET_STRING_fromBuf(&ie14->value.choice.RRCContainer, (const char *)f1ap_ue_context_setup_req->rrc_container,
                         f1ap_ue_context_setup_req->rrc_container_length);
  }

  /* OPTIONAL */
  /* MaskedIMEISV */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextSetupRequestIEs_t, ie15);
    ie15->id                             = F1AP_ProtocolIE_ID_id_MaskedIMEISV;
    ie15->criticality                    = F1AP_Criticality_reject;
    ie15->value.present                  = F1AP_UEContextSetupRequestIEs__value_PR_MaskedIMEISV;
    MaskedIMEISV_TO_BIT_STRING(12340000l, &ie15->value.choice.MaskedIMEISV); // size (64)
  }

  /* encode */
  uint8_t  *buffer=NULL;
  uint32_t  len=0;

  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UE CONTEXT SETUP REQUEST\n");
    return -1;
  }

  // xer_fprint(stdout, &asn_DEF_F1AP_F1AP_PDU, (void *)pdu);
  // asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  // res = asn_encode_to_new_buffer(NULL, ATS_CANONICAL_XER, &asn_DEF_F1AP_F1AP_PDU, pdu);
  // buffer = res.buffer;
  // len = res.result.encoded;
  // if (res.result.encoded <= 0) {
  //   LOG_E(F1AP, "ASN1 message encoding failed (%s, %lu)!\n", res.result.failed_type->name, res.result.encoded);
  //   return -1;
  // }
  LOG_D(F1AP,"F1AP UEContextSetupRequest Encoded %u bits\n", len);
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int CU_handle_UE_CONTEXT_SETUP_RESPONSE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  MessageDef                       *msg_p;
  F1AP_UEContextSetupResponse_t    *container;
  F1AP_UEContextSetupResponseIEs_t *ie;
  DevAssert(pdu);
  msg_p = itti_alloc_new_message(TASK_DU_F1, 0,  F1AP_UE_CONTEXT_SETUP_RESP);
  msg_p->ittiMsgHeader.originInstance = assoc_id;
  f1ap_ue_context_setup_t *f1ap_ue_context_setup_resp = &F1AP_UE_CONTEXT_SETUP_RESP(msg_p);
  container = &pdu->choice.successfulOutcome->value.choice.UEContextSetupResponse;
  int i;
  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  f1ap_ue_context_setup_resp->gNB_CU_ue_id = ie->value.choice.GNB_CU_UE_F1AP_ID;
  LOG_D(F1AP, "f1ap_ue_context_setup_resp->gNB_CU_ue_id is: %d \n", f1ap_ue_context_setup_resp->gNB_CU_ue_id);
  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  f1ap_ue_context_setup_resp->gNB_DU_ue_id = ie->value.choice.GNB_DU_UE_F1AP_ID;
  LOG_D(F1AP, "f1ap_ue_context_setup_resp->gNB_DU_ue_id is: %d \n", f1ap_ue_context_setup_resp->gNB_DU_ue_id);
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container, F1AP_ProtocolIE_ID_id_C_RNTI, false);
  if (ie) {
    f1ap_ue_context_setup_resp->crnti = calloc(1, sizeof(uint16_t));
    *f1ap_ue_context_setup_resp->crnti = ie->value.choice.C_RNTI;
  }

  // DUtoCURRCInformation
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_DUtoCURRCInformation, true);

  if (ie == NULL) {
    LOG_E(F1AP,"%s %d: ie is a NULL pointer \n",__FILE__,__LINE__);
    return -1;
  }

  f1ap_ue_context_setup_resp->du_to_cu_rrc_information = (du_to_cu_rrc_information_t *)calloc(1,sizeof(du_to_cu_rrc_information_t));
  f1ap_ue_context_setup_resp->du_to_cu_rrc_information->cellGroupConfig = (uint8_t *)calloc(1,ie->value.choice.DUtoCURRCInformation.cellGroupConfig.size);
  memcpy(f1ap_ue_context_setup_resp->du_to_cu_rrc_information->cellGroupConfig, ie->value.choice.DUtoCURRCInformation.cellGroupConfig.buf, ie->value.choice.DUtoCURRCInformation.cellGroupConfig.size);
  f1ap_ue_context_setup_resp->du_to_cu_rrc_information->cellGroupConfig_length = ie->value.choice.DUtoCURRCInformation.cellGroupConfig.size;
  // DRBs_Setup_List
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_DRBs_Setup_List, false);

  if(ie!=NULL) {
    f1ap_ue_context_setup_resp->drbs_to_be_setup_length = ie->value.choice.DRBs_Setup_List.list.count;
    f1ap_ue_context_setup_resp->drbs_to_be_setup = calloc(f1ap_ue_context_setup_resp->drbs_to_be_setup_length,
        sizeof(f1ap_drb_to_be_setup_t));
    AssertFatal(f1ap_ue_context_setup_resp->drbs_to_be_setup,
                "could not allocate memory for f1ap_ue_context_setup_resp->drbs_setup\n");

    for (i = 0; i < f1ap_ue_context_setup_resp->drbs_to_be_setup_length; ++i) {
      f1ap_drb_to_be_setup_t *drb_p = &f1ap_ue_context_setup_resp->drbs_to_be_setup[i];
      F1AP_DRBs_Setup_Item_t *drbs_setup_item_p;
      drbs_setup_item_p = &((F1AP_DRBs_Setup_ItemIEs_t *)ie->value.choice.DRBs_Setup_List.list.array[i])->value.choice.DRBs_Setup_Item;
      drb_p->drb_id = drbs_setup_item_p->dRBID;
      // TODO in the following, assume only one UP UL TNL is present.
      // this matches/assumes OAI CU/DU implementation, can be up to 2!
      drb_p->up_dl_tnl_length = 1;
      AssertFatal(drbs_setup_item_p->dLUPTNLInformation_ToBeSetup_List.list.count > 0,
                  "no DL UP TNL Information in DRBs to be Setup list\n");
      F1AP_DLUPTNLInformation_ToBeSetup_Item_t *dl_up_tnl_info_p = (F1AP_DLUPTNLInformation_ToBeSetup_Item_t *)drbs_setup_item_p->dLUPTNLInformation_ToBeSetup_List.list.array[0];
      F1AP_GTPTunnel_t *dl_up_tnl0 = dl_up_tnl_info_p->dLUPTNLInformation.choice.gTPTunnel;
      BIT_STRING_TO_TRANSPORT_LAYER_ADDRESS_IPv4(&dl_up_tnl0->transportLayerAddress, drb_p->up_dl_tnl[0].tl_address);
      OCTET_STRING_TO_UINT32(&dl_up_tnl0->gTP_TEID, drb_p->up_dl_tnl[0].teid);
    }
  }

  // SRBs_FailedToBeSetup_List
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetup_List, false);

  if(ie!=NULL) {
    f1ap_ue_context_setup_resp->srbs_failed_to_be_setup_length = ie->value.choice.SRBs_FailedToBeSetup_List.list.count;
    f1ap_ue_context_setup_resp->srbs_failed_to_be_setup = calloc(f1ap_ue_context_setup_resp->srbs_failed_to_be_setup_length,
        sizeof(f1ap_rb_failed_to_be_setup_t));
    AssertFatal(f1ap_ue_context_setup_resp->srbs_failed_to_be_setup,
                "could not allocate memory for f1ap_ue_context_setup_resp->srbs_failed_to_be_setup\n");

    for (i = 0; i < f1ap_ue_context_setup_resp->srbs_failed_to_be_setup_length; ++i) {
      f1ap_rb_failed_to_be_setup_t *srb_p = &f1ap_ue_context_setup_resp->srbs_failed_to_be_setup[i];
      srb_p->rb_id = ((F1AP_SRBs_FailedToBeSetup_Item_t *)ie->value.choice.SRBs_FailedToBeSetup_List.list.array[i])->sRBID;
    }
  }

  // DRBs_FailedToBeSetup_List
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetup_List, false);

  if(ie!=NULL) {
    f1ap_ue_context_setup_resp->drbs_failed_to_be_setup_length = ie->value.choice.DRBs_FailedToBeSetup_List.list.count;
    f1ap_ue_context_setup_resp->drbs_failed_to_be_setup = calloc(f1ap_ue_context_setup_resp->drbs_failed_to_be_setup_length,
        sizeof(f1ap_rb_failed_to_be_setup_t));
    AssertFatal(f1ap_ue_context_setup_resp->drbs_failed_to_be_setup,
                "could not allocate memory for f1ap_ue_context_setup_resp->drbs_failed_to_be_setup\n");

    for (i = 0; i < f1ap_ue_context_setup_resp->drbs_failed_to_be_setup_length; ++i) {
      f1ap_rb_failed_to_be_setup_t *drb_p = &f1ap_ue_context_setup_resp->drbs_failed_to_be_setup[i];
      drb_p->rb_id = ((F1AP_DRBs_FailedToBeSetup_Item_t *)ie->value.choice.DRBs_FailedToBeSetup_List.list.array[i])->dRBID;
    }
  }

  // SCell_FailedtoSetup_List
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_SCell_FailedtoSetup_List, false);

  if(ie!=NULL) {
    LOG_E (F1AP, "Not supporting handling of SCell_FailedtoSetup_List \n");
  }

  // SRBs_Setup_List
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextSetupResponseIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_SRBs_Setup_List, false);

  if(ie!=NULL) {
    f1ap_ue_context_setup_resp->srbs_to_be_setup_length = ie->value.choice.SRBs_Setup_List.list.count;
    f1ap_ue_context_setup_resp->srbs_to_be_setup = calloc(f1ap_ue_context_setup_resp->srbs_to_be_setup_length,
        sizeof(f1ap_srb_to_be_setup_t));
    AssertFatal(f1ap_ue_context_setup_resp->srbs_to_be_setup,
                "could not allocate memory for f1ap_ue_context_setup_resp->drbs_setup\n");

    for (i = 0; i < f1ap_ue_context_setup_resp->srbs_to_be_setup_length; ++i) {
      f1ap_srb_to_be_setup_t *srb_p = &f1ap_ue_context_setup_resp->srbs_to_be_setup[i];
      F1AP_SRBs_Setup_Item_t *srbs_setup_item_p;
      srbs_setup_item_p = &((F1AP_SRBs_Setup_ItemIEs_t *)ie->value.choice.SRBs_Setup_List.list.array[i])->value.choice.SRBs_Setup_Item;
      srb_p->srb_id = srbs_setup_item_p->sRBID;
      srb_p->lcid = srbs_setup_item_p->lCID;
    }
  }

  itti_send_msg_to_task(TASK_RRC_GNB, instance, msg_p);
  return 0;
}

int CU_handle_UE_CONTEXT_SETUP_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_handle_UE_CONTEXT_RELEASE_REQUEST(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  MessageDef *msg = itti_alloc_new_message(TASK_CU_F1, 0,  F1AP_UE_CONTEXT_RELEASE_REQ);
  msg->ittiMsgHeader.originInstance = assoc_id;
  f1ap_ue_context_release_req_t *req = &F1AP_UE_CONTEXT_RELEASE_REQ(msg);
  F1AP_UEContextReleaseRequest_t    *container;
  F1AP_UEContextReleaseRequestIEs_t *ie;
  DevAssert(pdu);
  container = &pdu->choice.initiatingMessage->value.choice.UEContextReleaseRequest;
  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  req->gNB_CU_ue_id = ie->value.choice.GNB_CU_UE_F1AP_ID;

  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  req->gNB_DU_ue_id = ie->value.choice.GNB_DU_UE_F1AP_ID;

  /* Cause */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseRequestIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_Cause, true);

  switch(ie->value.choice.Cause.present)
  {
    case F1AP_Cause_PR_radioNetwork:
      req->cause = F1AP_CAUSE_RADIO_NETWORK;
      req->cause_value = ie->value.choice.Cause.choice.radioNetwork;
      break;
    case F1AP_Cause_PR_transport:
      req->cause = F1AP_CAUSE_TRANSPORT;
      req->cause_value = ie->value.choice.Cause.choice.transport;
      break;
    case F1AP_Cause_PR_protocol:
      req->cause = F1AP_CAUSE_PROTOCOL;
      req->cause_value = ie->value.choice.Cause.choice.protocol;
      break;
    case F1AP_Cause_PR_misc:
      req->cause = F1AP_CAUSE_MISC;
      req->cause_value = ie->value.choice.Cause.choice.misc;
      break;
    case F1AP_Cause_PR_NOTHING:
    default:
      req->cause = F1AP_CAUSE_NOTHING;
      break;
  }

  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseRequestIEs_t, ie, container, F1AP_ProtocolIE_ID_id_targetCellsToCancel, false);
  if (ie != NULL) {
    LOG_W(F1AP, "ignoring list of target cells to cancel in UE Context Release Request: implementation missing\n");
  }

  itti_send_msg_to_task(TASK_RRC_GNB, instance, msg);

  return 0;
}

int CU_send_UE_CONTEXT_RELEASE_COMMAND(sctp_assoc_t assoc_id, f1ap_ue_context_release_cmd_t *cmd)
{
  F1AP_F1AP_PDU_t                   pdu= {0};
  F1AP_UEContextReleaseCommand_t    *out;
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextRelease;
  tmp->criticality   = F1AP_Criticality_reject;
  tmp->value.present = F1AP_InitiatingMessage__value_PR_UEContextReleaseCommand;
  out = &tmp->value.choice.UEContextReleaseCommand;
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextReleaseCommandIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_UEContextReleaseCommandIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = cmd->gNB_CU_ue_id;
  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextReleaseCommandIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_UEContextReleaseCommandIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = cmd->gNB_DU_ue_id;
  /* mandatory */
  /* c3. Cause */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextReleaseCommandIEs_t, ie3);
  ie3->id                             = F1AP_ProtocolIE_ID_id_Cause;
  ie3->criticality                    = F1AP_Criticality_ignore;
  ie3->value.present                  = F1AP_UEContextReleaseCommandIEs__value_PR_Cause;

  switch (cmd->cause) {
    case F1AP_CAUSE_RADIO_NETWORK:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_radioNetwork;
      ie3->value.choice.Cause.choice.radioNetwork = cmd->cause_value;
      break;

    case F1AP_CAUSE_TRANSPORT:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_transport;
      ie3->value.choice.Cause.choice.transport = cmd->cause_value;
      break;

    case F1AP_CAUSE_PROTOCOL:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_protocol;
      ie3->value.choice.Cause.choice.protocol = cmd->cause_value;
      break;

    case F1AP_CAUSE_MISC:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_misc;
      ie3->value.choice.Cause.choice.misc = cmd->cause_value;
      break;

    case F1AP_CAUSE_NOTHING:
    default:
      ie3->value.choice.Cause.present = F1AP_Cause_PR_NOTHING;
      break;
  }

  /* optional */
  /* c4. RRCContainer */
  if(cmd->rrc_container!=NULL){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextReleaseCommandIEs_t, ie4);
    ie4->id                             = F1AP_ProtocolIE_ID_id_RRCContainer;
    ie4->criticality                    = F1AP_Criticality_ignore;
    ie4->value.present                  = F1AP_UEContextReleaseCommandIEs__value_PR_RRCContainer;
    OCTET_STRING_fromBuf(&ie4->value.choice.RRCContainer, (const char *)cmd->rrc_container,
                       cmd->rrc_container_length);

    // conditionally have SRBID if RRC Container
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextReleaseCommandIEs_t, ie5);
    ie5->id = F1AP_ProtocolIE_ID_id_SRBID;
    ie5->criticality = F1AP_Criticality_ignore;
    ie5->value.present = F1AP_UEContextReleaseCommandIEs__value_PR_SRBID;
    ie5->value.choice.SRBID = cmd->srb_id;
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 context release command\n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int CU_handle_UE_CONTEXT_RELEASE_COMPLETE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  F1AP_UEContextReleaseComplete_t    *container;
  F1AP_UEContextReleaseCompleteIEs_t *ie;
  DevAssert(pdu);
  MessageDef *msg_p = itti_alloc_new_message(TASK_DU_F1, 0,  F1AP_UE_CONTEXT_RELEASE_COMPLETE);
  msg_p->ittiMsgHeader.originInstance = assoc_id;
  f1ap_ue_context_release_complete_t *complete = &F1AP_UE_CONTEXT_RELEASE_COMPLETE(msg_p);
  container = &pdu->choice.successfulOutcome->value.choice.UEContextReleaseComplete;
  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCompleteIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  complete->gNB_CU_ue_id = ie->value.choice.GNB_CU_UE_F1AP_ID;
  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCompleteIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  complete->gNB_DU_ue_id = ie->value.choice.GNB_DU_UE_F1AP_ID;

  /* Optional*/
  /* CriticalityDiagnostics */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextReleaseCompleteIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_CriticalityDiagnostics, false);

  if (ie) {
    // ie->value.choice.CriticalityDiagnostics.procedureCode
    // ie->value.choice.CriticalityDiagnostics.triggeringMessage
    // ie->value.choice.CriticalityDiagnostics.procedureCriticality
    // ie->value.choice.CriticalityDiagnostics.transactionID
    // F1AP_CriticalityDiagnostics_IE_List
  }

  itti_send_msg_to_task(TASK_RRC_GNB, instance, msg_p);

  return 0;
}

int CU_send_UE_CONTEXT_MODIFICATION_REQUEST(sctp_assoc_t assoc_id, f1ap_ue_context_modif_req_t *f1ap_ue_context_modification_req)
{
  F1AP_F1AP_PDU_t                        pdu= {0};
  F1AP_UEContextModificationRequest_t    *out;
  uint8_t  *buffer=NULL;
  uint32_t  len=0;

  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextModification;
  tmp->criticality   = F1AP_Criticality_reject;
  tmp->value.present = F1AP_InitiatingMessage__value_PR_UEContextModificationRequest;
  out = &tmp->value.choice.UEContextModificationRequest;
  /* mandatory */
  /* c1. GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie1);
  ie1->id                             = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality                    = F1AP_Criticality_reject;
  ie1->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = f1ap_ue_context_modification_req->gNB_CU_ue_id;
  /* mandatory */
  /* c2. GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie2);
  ie2->id                             = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality                    = F1AP_Criticality_reject;
  ie2->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = f1ap_ue_context_modification_req->gNB_DU_ue_id;

  /* optional */
  /* c3. NRCGI */
  if (true) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie3);
    ie3->id                             = F1AP_ProtocolIE_ID_id_SpCell_ID;
    ie3->criticality                    = F1AP_Criticality_ignore;
    ie3->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_NRCGI;
    /* - nRCGI */

    f1ap_served_cell_info_t nrcgi = {
        .plmn = f1ap_ue_context_modification_req->plmn,
        .nr_cellid = f1ap_ue_context_modification_req->nr_cellid,
    };
    addnRCGI(ie3->value.choice.NRCGI, &nrcgi);
  }

  /* optional */
  /* c4. ServCellIndex */
  if(0){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie4);
    ie4->id                             = F1AP_ProtocolIE_ID_id_ServCellIndex;
    ie4->criticality                    = F1AP_Criticality_reject;
    ie4->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_ServCellIndex;
    ie4->value.choice.ServCellIndex     = 5L;
  }

  /* optional */
  /* c5. DRXCycle */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie5);
    ie5->id                             = F1AP_ProtocolIE_ID_id_DRXCycle;
    ie5->criticality                    = F1AP_Criticality_ignore;
    ie5->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRXCycle;
    ie5->value.choice.DRXCycle.longDRXCycleLength = F1AP_LongDRXCycleLength_ms10; // enum

    if (0) {
      asn1cCallocOne(ie5->value.choice.DRXCycle.shortDRXCycleLength,
                     F1AP_ShortDRXCycleLength_ms2); // enum
    }

    if (0) {
      asn1cCallocOne(ie5->value.choice.DRXCycle.shortDRXCycleTimer, 123L);
    }
  }

  /* optional */
  /* c6. CUtoDURRCInformation */
  if (f1ap_ue_context_modification_req->cu_to_du_rrc_information!=NULL) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie6);
    ie6->id                             = F1AP_ProtocolIE_ID_id_CUtoDURRCInformation;
    ie6->criticality                    = F1AP_Criticality_reject;
    ie6->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_CUtoDURRCInformation;
    /* 6.1 cG_ConfigInfo */
    if(f1ap_ue_context_modification_req->cu_to_du_rrc_information->cG_ConfigInfo!=NULL){
      asn1cCalloc(ie6->value.choice.CUtoDURRCInformation.cG_ConfigInfo, cG_ConfigInfo);
      OCTET_STRING_fromBuf(cG_ConfigInfo, (const char *)f1ap_ue_context_modification_req->cu_to_du_rrc_information->cG_ConfigInfo,
        f1ap_ue_context_modification_req->cu_to_du_rrc_information->cG_ConfigInfo_length);
    }
    /* optional */
    /* 6.2 uE_CapabilityRAT_ContainerList */
    if(f1ap_ue_context_modification_req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList!=NULL){
      asn1cCalloc(ie6->value.choice.CUtoDURRCInformation.uE_CapabilityRAT_ContainerList, uE_CapabilityRAT_ContainerList );
      OCTET_STRING_fromBuf(uE_CapabilityRAT_ContainerList, (const char *)f1ap_ue_context_modification_req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList,
          f1ap_ue_context_modification_req->cu_to_du_rrc_information->uE_CapabilityRAT_ContainerList_length) ;
    }
    /* optional */
    /* 6.3 measConfig */
    if(f1ap_ue_context_modification_req->cu_to_du_rrc_information->measConfig!=NULL){
      asn1cCalloc(ie6->value.choice.CUtoDURRCInformation.measConfig,  measConfig);
      OCTET_STRING_fromBuf(measConfig, (const char *)f1ap_ue_context_modification_req->cu_to_du_rrc_information->measConfig,
          f1ap_ue_context_modification_req->cu_to_du_rrc_information->measConfig_length);
    }
  }

  /* optional */
  /* c7. TransmissionActionIndicator */
  if (f1ap_ue_context_modification_req->transm_action_ind != NULL) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie7);
    ie7->id                                     = F1AP_ProtocolIE_ID_id_TransmissionActionIndicator;
    ie7->criticality                            = F1AP_Criticality_ignore;
    ie7->value.present                          = F1AP_UEContextModificationRequestIEs__value_PR_TransmissionActionIndicator;
    ie7->value.choice.TransmissionActionIndicator = *f1ap_ue_context_modification_req->transm_action_ind;
    // Stop is guaranteed to be 0, by restart might be different from 1
    static_assert((int)F1AP_TransmissionActionIndicator_restart == (int)TransmActionInd_RESTART,
                  "mismatch of ASN.1 and internal representation\n");
  }

  /* optional */
  /* c8. ResourceCoordinationTransferContainer */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie8);
    ie8->id                             = F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer;
    ie8->criticality                    = F1AP_Criticality_ignore;
    ie8->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_ResourceCoordinationTransferContainer;
    OCTET_STRING_fromBuf(&ie8->value.choice.ResourceCoordinationTransferContainer, "asdsa1d32sa1d31asd31as",
                         strlen("asdsa1d32sa1d31asd31as"));
  }

  /* optional */
  /* c7. RRCRconfigurationCompleteIndicator */
  if (f1ap_ue_context_modification_req->ReconfigComplOutcome == RRCreconf_success) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie71);
    ie71->id                             = F1AP_ProtocolIE_ID_id_RRCReconfigurationCompleteIndicator;
    ie71->criticality                    = F1AP_Criticality_ignore;
    ie71->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_RRCReconfigurationCompleteIndicator;
    ie71->value.choice.RRCReconfigurationCompleteIndicator = F1AP_RRCReconfigurationCompleteIndicator_true;
  }

  /* optional */
  /* c8. RRCContainer */
  if (f1ap_ue_context_modification_req->rrc_container != NULL) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie81);
    ie81->id                             = F1AP_ProtocolIE_ID_id_RRCContainer;
    ie81->criticality                    = F1AP_Criticality_ignore;
    ie81->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_RRCContainer;
    OCTET_STRING_fromBuf(&ie81->value.choice.RRCContainer, (const char*)f1ap_ue_context_modification_req->rrc_container,
        f1ap_ue_context_modification_req->rrc_container_length);
  }

  /* optional */
  /* c9. SCell_ToBeSetupMod_List */
  if(0){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie9);
    ie9->id                             = F1AP_ProtocolIE_ID_id_SCell_ToBeSetupMod_List;
    ie9->criticality                    = F1AP_Criticality_ignore;
    ie9->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SCell_ToBeSetupMod_List;

    for (int i=0; i<1; i++) {
      //
      asn1cSequenceAdd(ie9->value.choice.SCell_ToBeSetupMod_List.list,
                     F1AP_SCell_ToBeSetupMod_ItemIEs_t, scell_toBeSetupMod_item_ies);
      scell_toBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_ToBeSetupMod_Item;
      scell_toBeSetupMod_item_ies->criticality   = F1AP_Criticality_ignore;
      scell_toBeSetupMod_item_ies->value.present = F1AP_SCell_ToBeSetupMod_ItemIEs__value_PR_SCell_ToBeSetupMod_Item;
      /* 8.1 SCell_ToBeSetup_Item */
      F1AP_SCell_ToBeSetupMod_Item_t *scell_toBeSetupMod_item=
          &scell_toBeSetupMod_item_ies->value.choice.SCell_ToBeSetupMod_Item;
      //   /* - sCell_ID */
      f1ap_served_cell_info_t hardCoded = {0};
      addnRCGI(scell_toBeSetupMod_item->sCell_ID, &hardCoded);
      /* sCellIndex */
      scell_toBeSetupMod_item->sCellIndex = 6;  // issue here
      }
  }

  /* optional */
  /* c10. SCell_ToBeRemoved_List */
  if(0){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie10);
    ie10->id                             = F1AP_ProtocolIE_ID_id_SCell_ToBeRemoved_List;
    ie10->criticality                    = F1AP_Criticality_ignore;
    ie10->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SCell_ToBeRemoved_List;

    for (int i=0;  i<1; i++) {
      //
      asn1cSequenceAdd(ie10->value.choice.SCell_ToBeRemoved_List.list,
                     F1AP_SCell_ToBeRemoved_ItemIEs_t, scell_toBeRemoved_item_ies);
      scell_toBeRemoved_item_ies = (F1AP_SCell_ToBeRemoved_ItemIEs_t *)calloc(1, sizeof(F1AP_SCell_ToBeRemoved_ItemIEs_t));
      //memset((void *)&scell_toBeRemoved_item_ies, 0, sizeof(F1AP_SCell_ToBeRemoved_ItemIEs_t));
      scell_toBeRemoved_item_ies->id            = F1AP_ProtocolIE_ID_id_SCell_ToBeRemoved_Item;
      scell_toBeRemoved_item_ies->criticality   = F1AP_Criticality_ignore;
      scell_toBeRemoved_item_ies->value.present = F1AP_SCell_ToBeRemoved_ItemIEs__value_PR_SCell_ToBeRemoved_Item;
      /* 10.1 SCell_ToBeRemoved_Item */
      F1AP_SCell_ToBeRemoved_Item_t *scell_toBeRemoved_item=
        &scell_toBeRemoved_item_ies->value.choice.SCell_ToBeRemoved_Item;
      /* - sCell_ID */
      f1ap_served_cell_info_t hardCoded = {0};
      addnRCGI(scell_toBeRemoved_item->sCell_ID, &hardCoded);
    }
  }

  /* mandatory */
  /* c11. SRBs_ToBeSetupMod_List */
  if(f1ap_ue_context_modification_req->srbs_to_be_setup_length > 0){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie11);
    ie11->id                             = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetupMod_List;
    ie11->criticality                    = F1AP_Criticality_reject;
    ie11->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SRBs_ToBeSetupMod_List;

    for (int i=0; i<f1ap_ue_context_modification_req->srbs_to_be_setup_length; i++) {
      //
      asn1cSequenceAdd(ie11->value.choice.SRBs_ToBeSetupMod_List, F1AP_SRBs_ToBeSetupMod_ItemIEs_t, srbs_toBeSetupMod_item_ies);
      srbs_toBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_ToBeSetupMod_Item; // 73
      srbs_toBeSetupMod_item_ies->criticality   = F1AP_Criticality_reject;
      srbs_toBeSetupMod_item_ies->value.present = F1AP_SRBs_ToBeSetupMod_ItemIEs__value_PR_SRBs_ToBeSetupMod_Item;
      /* 11.1 SRBs_ToBeSetup_Item */
      F1AP_SRBs_ToBeSetupMod_Item_t *srbs_toBeSetupMod_item=&srbs_toBeSetupMod_item_ies->value.choice.SRBs_ToBeSetupMod_Item;
      /* 11.1.1 sRBID */
      srbs_toBeSetupMod_item->sRBID = f1ap_ue_context_modification_req->srbs_to_be_setup[i].srb_id;
      /* OPTIONAL */
      /* 11.1.2 duplicationIndication */
      asn1cCallocOne(srbs_toBeSetupMod_item->duplicationIndication,
                   F1AP_DuplicationIndication_true); // enum
    }
  }

  /* mandatory */
  /* c12. DRBs_ToBeSetupMod_List */
  if(f1ap_ue_context_modification_req->drbs_to_be_setup_length){
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie12);
    ie12->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetupMod_List;
    ie12->criticality                    = F1AP_Criticality_reject;
    ie12->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRBs_ToBeSetupMod_List;

    for (int i = 0; i < f1ap_ue_context_modification_req->drbs_to_be_setup_length; i++) {
      const f1ap_drb_to_be_setup_t *drb = &f1ap_ue_context_modification_req->drbs_to_be_setup[i];
      asn1cSequenceAdd(ie12->value.choice.DRBs_ToBeSetupMod_List.list,
                     F1AP_DRBs_ToBeSetupMod_ItemIEs_t, drbs_toBeSetupMod_item_ies);
      drbs_toBeSetupMod_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeSetupMod_Item;
      drbs_toBeSetupMod_item_ies->criticality   = F1AP_Criticality_reject;
      drbs_toBeSetupMod_item_ies->value.present = F1AP_DRBs_ToBeSetupMod_ItemIEs__value_PR_DRBs_ToBeSetupMod_Item;
      /* 12.1 DRBs_ToBeSetupMod_Item */
      F1AP_DRBs_ToBeSetupMod_Item_t *drbs_toBeSetupMod_item=
          &drbs_toBeSetupMod_item_ies->value.choice.DRBs_ToBeSetupMod_Item;
      /* dRBID */
      drbs_toBeSetupMod_item->dRBID = drb->drb_id;
      /* qoSInformation */

      if(f1ap_ue_context_modification_req->QoS_information_type == EUTRAN_QoS){
        drbs_toBeSetupMod_item->qoSInformation.present = F1AP_QoSInformation_PR_eUTRANQoS;
        drbs_toBeSetupMod_item->qoSInformation.choice.eUTRANQoS = (F1AP_EUTRANQoS_t *)calloc(1, sizeof(F1AP_EUTRANQoS_t));
        drbs_toBeSetupMod_item->qoSInformation.choice.eUTRANQoS->qCI = 253L;
        /* uLUPTNLInformation_ToBeSetup_List */
        int maxnoofULTunnels = 1; // 2;

        for (int j=0;  j<maxnoofULTunnels;  j++) {
          /*  ULTunnels_ToBeSetup_Item */
          asn1cSequenceAdd( drbs_toBeSetupMod_item->uLUPTNLInformation_ToBeSetup_List.list,
                        F1AP_ULUPTNLInformation_ToBeSetup_Item_t, uLUPTNLInformation_ToBeSetup_Item);
          uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
          asn1cCalloc(uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.choice.gTPTunnel,
                      gTPTunnel);
          /* transportLayerAddress */
          TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);
          /* gTP_TEID */
          OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "4567",
            strlen("4567"));
        }

        /* rLCMode */
        drbs_toBeSetupMod_item->rLCMode = F1AP_RLCMode_rlc_um_bidirectional; // enum

        /* OPTIONAL */
        /* ULConfiguration */
        if (0) {
          drbs_toBeSetupMod_item->uLConfiguration = (F1AP_ULConfiguration_t *)calloc(1, sizeof(F1AP_ULConfiguration_t));
        }
      } //QoS information

      else{
        /* 12.1.2 DRB_Information */
        f1ap_drb_information_t *drb_info_in = &f1ap_ue_context_modification_req->drbs_to_be_setup[i].drb_info;
        drbs_toBeSetupMod_item->qoSInformation.present = F1AP_QoSInformation_PR_choice_extension;
        F1AP_QoSInformation_ExtIEs_t *ie = (F1AP_QoSInformation_ExtIEs_t *)calloc(1, sizeof(*ie));
        ie->id                             = F1AP_ProtocolIE_ID_id_DRB_Information;
        ie->criticality                    = F1AP_Criticality_reject;
        ie->value.present                  = F1AP_QoSInformation_ExtIEs__value_PR_DRB_Information;
        F1AP_DRB_Information_t   *DRB_Information = &ie->value.choice.DRB_Information;
        drbs_toBeSetupMod_item->qoSInformation.choice.choice_extension = (struct F1AP_ProtocolIE_SingleContainer *)ie;

        /* 12.1.2.1 dRB_QoS */
        f1ap_write_drb_qos_param(&drb_info_in->drb_qos, &DRB_Information->dRB_QoS);

        /* 12.1.2.2 sNSSAI */
        f1ap_write_drb_nssai(&drb->nssai, &DRB_Information->sNSSAI);

        /* OPTIONAL */
        /* 12.1.2.3 notificationControl */
        if (0) {
          asn1cCallocOne(DRB_Information->notificationControl,
              F1AP_NotificationControl_active); // enum
        }

        /* 12.1.2.4 flows_Mapped_To_DRB_List */
        f1ap_write_flows_mapped(drb_info_in->flows_mapped_to_drb, &DRB_Information->flows_Mapped_To_DRB_List, drb_info_in->flows_to_be_setup_length);

      } //QoS information

      /* 12.1.3 uLUPTNLInformation_ToBeSetup_List */
      for (int j = 0; j < f1ap_ue_context_modification_req->drbs_to_be_setup[i].up_ul_tnl_length; j++) {
        /* Use a dummy address and teid for the outgoing GTP-U tunnel (DU) which will be updated once we get the UE context modification response from the DU */
        asn1cSequenceAdd(drbs_toBeSetupMod_item->uLUPTNLInformation_ToBeSetup_List.list,
                       F1AP_ULUPTNLInformation_ToBeSetup_Item_t, uLUPTNLInformation_ToBeSetup_Item);
        uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
        asn1cCalloc( uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.choice.gTPTunnel,
                   gTPTunnel);
        /* 12.3.1.1.1 transportLayerAddress */
        TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(f1ap_ue_context_modification_req->drbs_to_be_setup[i].up_ul_tnl[j].tl_address,
                          &gTPTunnel->transportLayerAddress);
        /* 12.3.1.1.2 gTP_TEID */
        INT32_TO_OCTET_STRING(f1ap_ue_context_modification_req->drbs_to_be_setup[i].up_ul_tnl[j].teid,
                &gTPTunnel->gTP_TEID);
      }
      /* 12.1.4 rLCMode */
      /* TODO use rlc_mode from f1ap_drb_to_be_setup */
      switch (f1ap_ue_context_modification_req->drbs_to_be_setup[i].rlc_mode) {
        case RLC_MODE_AM:
          drbs_toBeSetupMod_item->rLCMode = F1AP_RLCMode_rlc_am;
          break;

        default:
          drbs_toBeSetupMod_item->rLCMode = F1AP_RLCMode_rlc_um_bidirectional;
      }

      /* OPTIONAL */
      /* 12.1.5 ULConfiguration */
      if (0) {
        asn1cCalloc(drbs_toBeSetupMod_item->uLConfiguration, tmp);
        tmp->uLUEConfiguration = F1AP_ULUEConfiguration_no_data;
      }

      /* OPTIONAL */
      /* 12.1.6 duplicationActivation */
      if (0) {
        asn1cCalloc(drbs_toBeSetupMod_item->duplicationActivation, tmp);
        *tmp = F1AP_DuplicationActivation_active;  // enum
      }
    }
  }

  /* optional */
  if(0){
    /* c13. DRBs_ToBeModified_List */
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie13);
    ie13->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeModified_List;
    ie13->criticality                    = F1AP_Criticality_reject;
    ie13->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRBs_ToBeModified_List;

    for (int i=0;   i<1; i++) {
      //
      asn1cSequenceAdd( ie13->value.choice.DRBs_ToBeModified_List.list,
                      F1AP_DRBs_ToBeModified_ItemIEs_t, drbs_toBeModified_item_ies);
      drbs_toBeModified_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeModified_Item;
      drbs_toBeModified_item_ies->criticality   = F1AP_Criticality_reject;
      drbs_toBeModified_item_ies->value.present = F1AP_DRBs_ToBeModified_ItemIEs__value_PR_DRBs_ToBeModified_Item;
      /* 13.1 SRBs_ToBeModified_Item */
      F1AP_DRBs_ToBeModified_Item_t *drbs_toBeModified_item=
          &drbs_toBeModified_item_ies->value.choice.DRBs_ToBeModified_Item;
      /* dRBID */
      drbs_toBeModified_item->dRBID = 30L;
      /* qoSInformation */
      asn1cCalloc(drbs_toBeModified_item->qoSInformation, tmp);
      tmp->present = F1AP_QoSInformation_PR_eUTRANQoS;
      tmp->choice.eUTRANQoS = (F1AP_EUTRANQoS_t *)calloc(1, sizeof(F1AP_EUTRANQoS_t));
      tmp->choice.eUTRANQoS->qCI = 254L;
      /* ULTunnels_ToBeModified_List */
      int maxnoofULTunnels = 1; // 2;

      for (int j=0; j<maxnoofULTunnels; j++) {
        /*  ULTunnels_ToBeModified_Item */
        asn1cSequenceAdd(drbs_toBeModified_item->uLUPTNLInformation_ToBeSetup_List.list,
                       F1AP_ULUPTNLInformation_ToBeSetup_Item_t, uLUPTNLInformation_ToBeSetup_Item);
        uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.present = F1AP_UPTransportLayerInformation_PR_gTPTunnel;
        asn1cCalloc(uLUPTNLInformation_ToBeSetup_Item->uLUPTNLInformation.choice.gTPTunnel,
                  gTPTunnel);
        TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &gTPTunnel->transportLayerAddress);
        OCTET_STRING_fromBuf(&gTPTunnel->gTP_TEID, "1204",
                           strlen("1204"));
      }

      /* OPTIONAL */
      /* ULConfiguration */
      if (0) {
        drbs_toBeModified_item->uLConfiguration = (F1AP_ULConfiguration_t *)calloc(1, sizeof(F1AP_ULConfiguration_t));
      }
    }
  }

  /* optional */
  if(0){
    /* c14. SRBs_ToBeReleased_List */
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie14);
    ie14->id                             = F1AP_ProtocolIE_ID_id_SRBs_ToBeReleased_List;
    ie14->criticality                    = F1AP_Criticality_reject;
    ie14->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_SRBs_ToBeReleased_List;

    for (int i=0;  i<1; i++) {
      //
      asn1cSequenceAdd(ie14->value.choice.SRBs_ToBeReleased_List.list,
                     F1AP_SRBs_ToBeReleased_ItemIEs_t, srbs_toBeReleased_item_ies);
      srbs_toBeReleased_item_ies->id            = F1AP_ProtocolIE_ID_id_SRBs_ToBeReleased_Item;
      srbs_toBeReleased_item_ies->criticality   = F1AP_Criticality_ignore;
      srbs_toBeReleased_item_ies->value.present = F1AP_SRBs_ToBeReleased_ItemIEs__value_PR_SRBs_ToBeReleased_Item;
      /* 9.1 SRBs_ToBeReleased_Item */
      F1AP_SRBs_ToBeReleased_Item_t *srbs_toBeReleased_item=
          &srbs_toBeReleased_item_ies->value.choice.SRBs_ToBeReleased_Item;
      /* - sRBID */
      srbs_toBeReleased_item->sRBID = 2L;
    }
  }

  /* optional */
  if (f1ap_ue_context_modification_req->drbs_to_be_released_length > 0) {
    /* c15. DRBs_ToBeReleased_List */
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRequestIEs_t, ie15);
    ie15->id                             = F1AP_ProtocolIE_ID_id_DRBs_ToBeReleased_List;
    ie15->criticality                    = F1AP_Criticality_reject;
    ie15->value.present                  = F1AP_UEContextModificationRequestIEs__value_PR_DRBs_ToBeReleased_List;

    for (int i = 0; i < f1ap_ue_context_modification_req->drbs_to_be_released_length; i++) {
      //
      asn1cSequenceAdd(ie15->value.choice.DRBs_ToBeReleased_List.list,
                     F1AP_DRBs_ToBeReleased_ItemIEs_t, drbs_toBeReleased_item_ies);
      drbs_toBeReleased_item_ies->id            = F1AP_ProtocolIE_ID_id_DRBs_ToBeReleased_Item;
      drbs_toBeReleased_item_ies->criticality   = F1AP_Criticality_reject;
      drbs_toBeReleased_item_ies->value.present = F1AP_DRBs_ToBeReleased_ItemIEs__value_PR_DRBs_ToBeReleased_Item;
      /* 14.1 SRBs_ToBeReleased_Item */
      F1AP_DRBs_ToBeReleased_Item_t *drbs_toBeReleased_item=
          &drbs_toBeReleased_item_ies->value.choice.DRBs_ToBeReleased_Item;
      /* dRBID */
      drbs_toBeReleased_item->dRBID = f1ap_ue_context_modification_req->drbs_to_be_released[i].rb_id;
    }
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UE CONTEXT_MODIFICATION REQUEST\n");
    return -1;
  }
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int CU_handle_UE_CONTEXT_MODIFICATION_RESPONSE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  MessageDef                       *msg_p;
  F1AP_UEContextModificationResponse_t    *container;
  F1AP_UEContextModificationResponseIEs_t *ie;
  DevAssert(pdu);
  msg_p = itti_alloc_new_message(TASK_DU_F1, 0,  F1AP_UE_CONTEXT_MODIFICATION_RESP);
  msg_p->ittiMsgHeader.originInstance = assoc_id;
  f1ap_ue_context_modif_resp_t *f1ap_ue_context_modification_resp = &F1AP_UE_CONTEXT_MODIFICATION_RESP(msg_p);
  container = &pdu->choice.successfulOutcome->value.choice.UEContextModificationResponse;
  int i;

    /* GNB_CU_UE_F1AP_ID */
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationResponseIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
    f1ap_ue_context_modification_resp->gNB_CU_ue_id = ie->value.choice.GNB_CU_UE_F1AP_ID;

    LOG_D(F1AP, "f1ap_ue_context_modif_resp->gNB_CU_ue_id is: %d \n", f1ap_ue_context_modification_resp->gNB_CU_ue_id);

    /* GNB_DU_UE_F1AP_ID */
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationResponseIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
    f1ap_ue_context_modification_resp->gNB_DU_ue_id = ie->value.choice.GNB_DU_UE_F1AP_ID;

    LOG_D(F1AP, "f1ap_ue_context_modification_resp->gNB_DU_ue_id is: %d \n", f1ap_ue_context_modification_resp->gNB_DU_ue_id);

    // DUtoCURRCInformation
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationResponseIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_DUtoCURRCInformation, false);
    if(ie!=NULL){
      f1ap_ue_context_modification_resp->du_to_cu_rrc_information = (du_to_cu_rrc_information_t *)calloc(1, sizeof(du_to_cu_rrc_information_t));
      f1ap_ue_context_modification_resp->du_to_cu_rrc_information->cellGroupConfig = (uint8_t *)calloc(1,ie->value.choice.DUtoCURRCInformation.cellGroupConfig.size);

      memcpy(f1ap_ue_context_modification_resp->du_to_cu_rrc_information->cellGroupConfig, ie->value.choice.DUtoCURRCInformation.cellGroupConfig.buf, ie->value.choice.DUtoCURRCInformation.cellGroupConfig.size);
      f1ap_ue_context_modification_resp->du_to_cu_rrc_information->cellGroupConfig_length = ie->value.choice.DUtoCURRCInformation.cellGroupConfig.size;
    }

    // DRBs_SetupMod_List
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationResponseIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_DRBs_SetupMod_List, false);
    if(ie!=NULL){
      f1ap_ue_context_modification_resp->drbs_to_be_setup_length = ie->value.choice.DRBs_SetupMod_List.list.count;
      f1ap_ue_context_modification_resp->drbs_to_be_setup = calloc(f1ap_ue_context_modification_resp->drbs_to_be_setup_length,
          sizeof(f1ap_drb_to_be_setup_t));
      AssertFatal(f1ap_ue_context_modification_resp->drbs_to_be_setup,
                "could not allocate memory for f1ap_ue_context_setup_resp->drbs_setup\n");
      for (i = 0; i < f1ap_ue_context_modification_resp->drbs_to_be_setup_length; ++i) {
        f1ap_drb_to_be_setup_t *drb_p = &f1ap_ue_context_modification_resp->drbs_to_be_setup[i];
        F1AP_DRBs_SetupMod_Item_t *drbs_setupmod_item_p;
        drbs_setupmod_item_p = &((F1AP_DRBs_SetupMod_ItemIEs_t *)ie->value.choice.DRBs_SetupMod_List.list.array[i])->value.choice.DRBs_SetupMod_Item;
        drb_p->drb_id = drbs_setupmod_item_p->dRBID;
        // TODO in the following, assume only one UP UL TNL is present.
         // this matches/assumes OAI CU/DU implementation, can be up to 2!
        drb_p->up_dl_tnl_length = 1;
        AssertFatal(drbs_setupmod_item_p->dLUPTNLInformation_ToBeSetup_List.list.count > 0,
            "no DL UP TNL Information in DRBs to be Setup list\n");
        F1AP_DLUPTNLInformation_ToBeSetup_Item_t *dl_up_tnl_info_p = (F1AP_DLUPTNLInformation_ToBeSetup_Item_t *)drbs_setupmod_item_p->dLUPTNLInformation_ToBeSetup_List.list.array[0];
        F1AP_GTPTunnel_t *dl_up_tnl0 = dl_up_tnl_info_p->dLUPTNLInformation.choice.gTPTunnel;
        BIT_STRING_TO_TRANSPORT_LAYER_ADDRESS_IPv4(&dl_up_tnl0->transportLayerAddress, drb_p->up_dl_tnl[0].tl_address);
        OCTET_STRING_TO_UINT32(&dl_up_tnl0->gTP_TEID, drb_p->up_dl_tnl[0].teid);
      }
    }
    // SRBs_FailedToBeSetupMod_List
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationResponseIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_SRBs_FailedToBeSetupMod_List, false);
    if(ie!=NULL){
      f1ap_ue_context_modification_resp->srbs_failed_to_be_setup_length = ie->value.choice.SRBs_FailedToBeSetupMod_List.list.count;
      f1ap_ue_context_modification_resp->srbs_failed_to_be_setup = calloc(f1ap_ue_context_modification_resp->srbs_failed_to_be_setup_length,
          sizeof(f1ap_rb_failed_to_be_setup_t));
      AssertFatal(f1ap_ue_context_modification_resp->srbs_failed_to_be_setup,
          "could not allocate memory for f1ap_ue_context_setup_resp->srbs_failed_to_be_setup\n");
      for (i = 0; i < f1ap_ue_context_modification_resp->srbs_failed_to_be_setup_length; ++i) {
        f1ap_rb_failed_to_be_setup_t *srb_p = &f1ap_ue_context_modification_resp->srbs_failed_to_be_setup[i];
        srb_p->rb_id = ((F1AP_SRBs_FailedToBeSetupMod_Item_t *)ie->value.choice.SRBs_FailedToBeSetupMod_List.list.array[i])->sRBID;
      }

    }
    // DRBs_FailedToBeSetupMod_List
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationResponseIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_DRBs_FailedToBeSetupMod_List, false);
    if(ie!=NULL){
      f1ap_ue_context_modification_resp->drbs_failed_to_be_setup_length = ie->value.choice.DRBs_FailedToBeSetupMod_List.list.count;
      f1ap_ue_context_modification_resp->drbs_failed_to_be_setup = calloc(f1ap_ue_context_modification_resp->drbs_failed_to_be_setup_length,
          sizeof(f1ap_rb_failed_to_be_setup_t));
      AssertFatal(f1ap_ue_context_modification_resp->drbs_failed_to_be_setup,
          "could not allocate memory for f1ap_ue_context_setup_resp->drbs_failed_to_be_setup\n");
      for (i = 0; i < f1ap_ue_context_modification_resp->drbs_failed_to_be_setup_length; ++i) {
        f1ap_rb_failed_to_be_setup_t *drb_p = &f1ap_ue_context_modification_resp->drbs_failed_to_be_setup[i];
        drb_p->rb_id = ((F1AP_DRBs_FailedToBeSetupMod_Item_t *)ie->value.choice.DRBs_FailedToBeSetupMod_List.list.array[i])->dRBID;
      }
    }

    // SCell_FailedtoSetupMod_List
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationResponseIEs_t, ie, container,
                               F1AP_ProtocolIE_ID_id_SCell_FailedtoSetupMod_List, false);
    if(ie!=NULL){
      LOG_E (F1AP, "Not supporting handling of SCell_FailedtoSetupMod_List \n");
    }

    // SRBs_Setup_List
    F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationResponseIEs_t, ie, container,
        F1AP_ProtocolIE_ID_id_SRBs_SetupMod_List, false);
    if(ie!=NULL){
      f1ap_ue_context_modification_resp->srbs_to_be_setup_length = ie->value.choice.SRBs_SetupMod_List.list.count;
      f1ap_ue_context_modification_resp->srbs_to_be_setup = calloc(f1ap_ue_context_modification_resp->srbs_to_be_setup_length,
          sizeof(f1ap_srb_to_be_setup_t));
      AssertFatal(f1ap_ue_context_modification_resp->srbs_to_be_setup,
          "could not allocate memory for f1ap_ue_context_setup_resp->drbs_setup\n");
      for (i = 0; i < f1ap_ue_context_modification_resp->srbs_to_be_setup_length; ++i) {
        f1ap_srb_to_be_setup_t *srb_p = &f1ap_ue_context_modification_resp->srbs_to_be_setup[i];
        F1AP_SRBs_SetupMod_Item_t *srbs_setup_item_p;
        srbs_setup_item_p = &((F1AP_SRBs_SetupMod_ItemIEs_t *)ie->value.choice.SRBs_SetupMod_List.list.array[i])->value.choice.SRBs_SetupMod_Item;
        srb_p->srb_id = srbs_setup_item_p->sRBID;
        srb_p->lcid = srbs_setup_item_p->lCID;
      }
    }

    itti_send_msg_to_task(TASK_RRC_GNB, instance, msg_p);
    return 0;
}

int CU_handle_UE_CONTEXT_MODIFICATION_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
    AssertFatal(1 == 0, "Not implemented yet\n");
}

int CU_handle_UE_CONTEXT_MODIFICATION_REQUIRED(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  DevAssert(pdu != NULL);

  MessageDef *msg_p = itti_alloc_new_message(TASK_DU_F1, 0, F1AP_UE_CONTEXT_MODIFICATION_REQUIRED);
  msg_p->ittiMsgHeader.originInstance = assoc_id;
  f1ap_ue_context_modif_required_t *required = &F1AP_UE_CONTEXT_MODIFICATION_REQUIRED(msg_p);

  F1AP_UEContextModificationRequired_t *container = &pdu->choice.initiatingMessage->value.choice.UEContextModificationRequired;
  F1AP_UEContextModificationRequiredIEs_t *ie = NULL;

  /* required: GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t, ie, container, F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  required->gNB_CU_ue_id = ie->value.choice.GNB_CU_UE_F1AP_ID;

  /* required: GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t, ie, container, F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  required->gNB_DU_ue_id = ie->value.choice.GNB_DU_UE_F1AP_ID;

  /* optional: Resource Coordination Transfer Container */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer,
                             false);
  AssertFatal(ie == NULL, "handling of Resource Coordination Transfer Container not implemented\n");

  /* optional: DU to CU RRC Information */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_DUtoCURRCInformation,
                             false);
  if (ie != NULL) {
    F1AP_DUtoCURRCInformation_t *du2cu = &ie->value.choice.DUtoCURRCInformation;
    required->du_to_cu_rrc_information = malloc(sizeof(*required->du_to_cu_rrc_information));
    AssertFatal(required->du_to_cu_rrc_information != NULL, "memory allocation failed\n");

    required->du_to_cu_rrc_information->cellGroupConfig = malloc(du2cu->cellGroupConfig.size);
    AssertFatal(required->du_to_cu_rrc_information->cellGroupConfig != NULL, "memory allocation failed\n");
    memcpy(required->du_to_cu_rrc_information->cellGroupConfig, du2cu->cellGroupConfig.buf, du2cu->cellGroupConfig.size);
    required->du_to_cu_rrc_information->cellGroupConfig_length = du2cu->cellGroupConfig.size;

    AssertFatal(du2cu->measGapConfig == NULL, "handling of measGapConfig not implemented\n");
    AssertFatal(du2cu->requestedP_MaxFR1 == NULL, "handling of requestedP_MaxFR1 not implemented\n");
  }

  /* optional: DRB Required to Be Modified List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_DRBs_Required_ToBeModified_List,
                             false);
  AssertFatal(ie == NULL, "handling of DRBs Required to be modified list not implemented\n");

  /* optional: SRB Required to be Released List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_SRBs_Required_ToBeReleased_List,
                             false);
  AssertFatal(ie == NULL, "handling of SRBs Required to be released list not implemented\n");

  /* optional: DRB Required to be Released List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_DRBs_Required_ToBeReleased_List,
                             false);
  AssertFatal(ie == NULL, "handling of DRBs Required to be released list not implemented\n");

  /* mandatory: Cause */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t, ie, container, F1AP_ProtocolIE_ID_id_Cause, true);
  switch (ie->value.choice.Cause.present) {
    case F1AP_Cause_PR_radioNetwork:
      required->cause = F1AP_CAUSE_RADIO_NETWORK;
      required->cause_value = ie->value.choice.Cause.choice.radioNetwork;
      break;
    case F1AP_Cause_PR_transport:
      required->cause = F1AP_CAUSE_TRANSPORT;
      required->cause_value = ie->value.choice.Cause.choice.transport;
      break;
    case F1AP_Cause_PR_protocol:
      required->cause = F1AP_CAUSE_PROTOCOL;
      required->cause_value = ie->value.choice.Cause.choice.protocol;
      break;
    case F1AP_Cause_PR_misc:
      required->cause = F1AP_CAUSE_MISC;
      required->cause_value = ie->value.choice.Cause.choice.misc;
      break;
    default:
      LOG_W(F1AP, "Unknown cause for UE Context Modification required message\n");
      /* fall through */
    case F1AP_Cause_PR_NOTHING:
      required->cause = F1AP_CAUSE_NOTHING;
      break;
  }

  /* optional: BH RLC Channel Required to be Released List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_BHChannels_Required_ToBeReleased_List,
                             false);
  AssertFatal(ie == NULL, "handling of BH RLC Channel Required to be Released list not implemented\n");

  /* optional: SL DRB Required to Be Modified List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_SLDRBs_Required_ToBeModified_List,
                             false);
  AssertFatal(ie == NULL, "handling of SL DRB Required to be modified list not implemented\n");

  /* optional: SL DRB Required to be Released List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_SLDRBs_Required_ToBeReleased_List,
                             false);
  AssertFatal(ie == NULL, "handling of SL DRBs Required to be released list not implemented\n");

  /* optional: Candidate Cells To Be Cancelled List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_Candidate_SpCell_List,
                             false);
  AssertFatal(ie == NULL, "handling of candidate cells to be cancelled list not implemented\n");

  itti_send_msg_to_task(TASK_RRC_GNB, instance, msg_p);
  return 0;
}

int CU_send_UE_CONTEXT_MODIFICATION_CONFIRM(sctp_assoc_t assoc_id, f1ap_ue_context_modif_confirm_t *confirm)
{
  F1AP_F1AP_PDU_t pdu = {0};
  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextModificationRequired;
  tmp->criticality = F1AP_Criticality_reject;
  tmp->value.present = F1AP_SuccessfulOutcome__value_PR_UEContextModificationConfirm;
  F1AP_UEContextModificationConfirm_t *out = &tmp->value.choice.UEContextModificationConfirm;

  /* mandatory: GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationConfirmIEs_t, ie1);
  ie1->id = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality = F1AP_Criticality_reject;
  ie1->value.present = F1AP_UEContextModificationConfirmIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = confirm->gNB_CU_ue_id;

  /* mandatory: GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationConfirmIEs_t, ie2);
  ie2->id = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality = F1AP_Criticality_reject;
  ie2->value.present = F1AP_UEContextModificationConfirmIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = confirm->gNB_DU_ue_id;

  /* optional: Resource Coordination Transfer Container */
  /* not implemented*/

  /* optional: DRB Modified List */
  /* not implemented*/

  /* optional: RRC Container */
  if (confirm->rrc_container != NULL) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationConfirmIEs_t, ie);
    ie->id = F1AP_ProtocolIE_ID_id_RRCContainer;
    ie->criticality = F1AP_Criticality_ignore;
    ie->value.present = F1AP_UEContextModificationConfirmIEs__value_PR_RRCContainer;
    OCTET_STRING_fromBuf(&ie->value.choice.RRCContainer, (const char *)confirm->rrc_container, confirm->rrc_container_length);
  }

  /* optional: CriticalityDiagnostics */
  /* not implemented*/

  /* optional: Execute Duplication */
  /* not implemented*/

  /* optional: Resource Coordination Transfer Information */
  /* not implemented*/

  /* optional: SL DRB Modified List */
  /* not implemented*/

  /* encode */
  uint8_t *buffer = NULL;
  uint32_t len = 0;
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UE Context Modification Confirm\n");
    return -1;
  }
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int CU_send_UE_CONTEXT_MODIFICATION_REFUSE(sctp_assoc_t assoc_id, f1ap_ue_context_modif_refuse_t *refuse)
{
  F1AP_F1AP_PDU_t pdu = {0};
  pdu.present = F1AP_F1AP_PDU_PR_unsuccessfulOutcome;
  asn1cCalloc(pdu.choice.unsuccessfulOutcome, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextModificationRequired;
  tmp->criticality = F1AP_Criticality_reject;
  tmp->value.present = F1AP_UnsuccessfulOutcome__value_PR_UEContextModificationRefuse;
  F1AP_UEContextModificationRefuse_t *out = &tmp->value.choice.UEContextModificationRefuse;

  /* mandatory: GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRefuseIEs_t, ie1);
  ie1->id = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality = F1AP_Criticality_reject;
  ie1->value.present = F1AP_UEContextModificationRefuseIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = refuse->gNB_CU_ue_id;

  /* mandatory: GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRefuseIEs_t, ie2);
  ie2->id = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality = F1AP_Criticality_reject;
  ie2->value.present = F1AP_UEContextModificationRefuseIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = refuse->gNB_DU_ue_id;

  /* optional: Cause */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRefuseIEs_t, ie3);
  ie3->id = F1AP_ProtocolIE_ID_id_Cause;
  ie3->criticality = F1AP_Criticality_reject;
  ie3->value.present = F1AP_UEContextModificationRefuseIEs__value_PR_Cause;
  F1AP_Cause_t *cause = &ie3->value.choice.Cause;
  switch (refuse->cause) {
    case F1AP_CAUSE_RADIO_NETWORK:
      cause->present = F1AP_Cause_PR_radioNetwork;
      cause->choice.radioNetwork = refuse->cause_value;
      break;
    case F1AP_CAUSE_TRANSPORT:
      cause->present = F1AP_Cause_PR_transport;
      cause->choice.transport = refuse->cause_value;
      break;
    case F1AP_CAUSE_PROTOCOL:
      cause->present = F1AP_Cause_PR_protocol;
      cause->choice.protocol = refuse->cause_value;
      break;
    case F1AP_CAUSE_MISC:
      cause->present = F1AP_Cause_PR_misc;
      cause->choice.misc = refuse->cause_value;
      break;
    case F1AP_CAUSE_NOTHING:
    default:
      cause->present = F1AP_Cause_PR_NOTHING;
      break;
  } // switch

  /* optional: CriticalityDiagnostics */
  /* not implemented*/

  /* encode */
  uint8_t *buffer = NULL;
  uint32_t len = 0;
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UE Context Modification Refuse\n");
    return -1;
  }
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}
