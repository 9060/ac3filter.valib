/*
  DTS parser class
*/

#ifndef DTS_PARSER
#define DTS_PARSER

#include "spk.h"
#include "dts_defs.h"
#include "bitstream.h"
#include "parser.h"


class DTSInfo
{
public:
  // Frame header
  int frame_type;         // type of the current frame
  int samples_deficit;    // deficit sample count
  int crc_present;        // crc is present in the bitstream
  int sample_blocks;      // number of PCM sample blocks
  int amode;              // audio channels arrangement
  int sample_rate;        // audio sampling rate
  int bit_rate;           // transmission bit rate

  int downmix;            // embedded downmix enabled
  int dynrange;           // embedded dynamic range flag
  int timestamp;          // embedded time stamp flag
  int aux_data;           // auxiliary data flag
  int hdcd;               // source material is mastered in HDCD
  int ext_descr;          // extension audio descriptor flag
  int ext_coding;         // extended coding flag
  int aspf;               // audio sync word insertion flag
  int lfe;                // low frequency effects flag
  int predictor_history;  // predictor history flag
  int header_crc;         // header crc check bytes
  int multirate_inter;    // multirate interpolator switch
  int version;            // encoder software revision
  int copy_history;       // copy history
  int source_pcm_res;     // source pcm resolution
  int front_sum;          // front sum/difference flag
  int surround_sum;       // surround sum/difference flag
  int dialog_norm;        // dialog normalisation parameter

  // Primary audio coding header
  int subframes;          // number of subframes
  int prim_channels;      // number of primary audio channels
  
  int subband_activity[DTS_PRIM_CHANNELS_MAX];    // subband activity count
  int vq_start_subband[DTS_PRIM_CHANNELS_MAX];    // high frequency vq start subband
  int joint_intensity[DTS_PRIM_CHANNELS_MAX];     // joint intensity coding index
  int transient_huffman[DTS_PRIM_CHANNELS_MAX];   // transient mode code book
  int scalefactor_huffman[DTS_PRIM_CHANNELS_MAX]; // scale factor code book
  int bitalloc_huffman[DTS_PRIM_CHANNELS_MAX];    // bit allocation quantizer select
  int quant_index_huffman[DTS_PRIM_CHANNELS_MAX][DTS_ABITS_MAX]; // quantization index codebook select
  float scalefactor_adj[DTS_PRIM_CHANNELS_MAX][DTS_ABITS_MAX];   // scale factor adjustment

  // Primary audio coding side information
  int subsubframes;       // number of subsubframes
  int partial_samples;    // partial subsubframe samples count
  int prediction_mode[DTS_PRIM_CHANNELS_MAX][DTS_SUBBANDS];   // prediction mode (ADPCM used or not)
  int prediction_vq[DTS_PRIM_CHANNELS_MAX][DTS_SUBBANDS];     // prediction VQ coefs
  int bitalloc[DTS_PRIM_CHANNELS_MAX][DTS_SUBBANDS];          // bit allocation index
  int transition_mode[DTS_PRIM_CHANNELS_MAX][DTS_SUBBANDS];   // transition mode (transients)
  int scale_factor[DTS_PRIM_CHANNELS_MAX][DTS_SUBBANDS][2];   // scale factors (2 if transient)
  int joint_huff[DTS_PRIM_CHANNELS_MAX];                      // joint subband scale factors codebook
  int joint_scale_factor[DTS_PRIM_CHANNELS_MAX][DTS_SUBBANDS];// joint subband scale factors
  int downmix_coef[DTS_PRIM_CHANNELS_MAX][2];                 // stereo downmix coefficients
  int dynrange_coef;      // dynamic range coefficient

  // VQ encoded high frequency subbands
  int high_freq_vq[DTS_PRIM_CHANNELS_MAX][DTS_SUBBANDS];

  // Low frequency effect data
  double lfe_data[2*DTS_SUBSUBFAMES_MAX*DTS_LFE_MAX * 2 /*history*/];
  int lfe_scale_factor;

  // Subband samples history (for ADPCM)
  double subband_samples_hist[DTS_PRIM_CHANNELS_MAX][DTS_SUBBANDS][4];
  double subband_fir_hist[DTS_PRIM_CHANNELS_MAX][512];
  double subband_fir_noidea[DTS_PRIM_CHANNELS_MAX][64];
};

class DTSParser : public BaseParser, public DTSInfo
{
public:
  DTSParser();

  /////////////////////////////////////////////////////////
  // Parser overrides

  void reset();
  bool decode_frame();
  void get_info(char *buf, unsigned len) const;

protected:

  /////////////////////////////////////////////////////////
  // BaseParser overrides

  unsigned sync(uint8_t **buf, uint8_t *end);
  bool check_crc();
  bool start_decode();

  /////////////////////////////////////////////////////////
  // DTS parse

  // bitstream
  ReadBS    bs;

  int current_subframe;
  int current_subsubframe;

  // pre-calculated cosine modulation coefs for the QMF
  double cos_mod[544];
  void init_cosmod();

  // parse functions
  unsigned dts_sync(uint8_t *buf) const;
  bool parse_frame_header();
  bool parse_subframe_header();
  bool parse_subsubframe();
  bool parse_subframe_footer();

  // utility functions
  int  InverseQ(huff_entry_t *huff);
  void qmf_32_subbands(int ch, 
         double samples_in[32][8], 
         sample_t *samples_out,
         double scale);
  void lfe_interpolation_fir(int nDecimationSelect, 
         int nNumDeciSample,
         double *samples_in, 
         sample_t *samples_out,
         double scale);
};


#endif
