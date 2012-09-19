#ifndef SPDIF_DEFS_H
#define SPDIF_DEFS_H

enum spdif_type_t
{
  spdif_type_none = -1,
  spdif_type_null = 0,
  spdif_type_pause = 3,

  // Dolby
  spdif_type_ac3  = 1,
  spdif_type_eac3 = 21,
  // MPEG Audio
  spdif_type_mpeg1_layer1  = 4,
  spdif_type_mpeg1_layer23 = 5,
  spdif_type_mpeg2_ext     = 6,
  spdif_type_mpeg2_lsf_layer1 = 8,
  spdif_type_mpeg2_lsf_layer2 = 9,
  spdif_type_mpeg2_lsf_layer3 = 10,
  // AAC
  spdif_type_mpeg2_aac_adts = 7,
  // DTS
  spdif_type_dts_512  = 11,
  spdif_type_dts_1024 = 12,
  spdif_type_dts_2048 = 13,
  spdif_type_dts_hd   = 17,
  // ATRAC
  spdif_type_atrac   = 14,
  spdif_type_atrac23 = 15,
  spdif_type_atrac_x = 16
};

enum dts_conv_t
{
  dts_conv_none  = 0,
  dts_conv_16bit = 1,
  dts_conv_14bit = 2
};

enum dts_mode_t
{
  dts_mode_auto    = 0,
  dts_mode_wrapped = 1,
  dts_mode_padded  = 2
};

#endif
