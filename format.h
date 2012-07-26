static int sndfile_format_string2number(const char *str)
{
  if(!strcmp(str, "WAV")) /* Microsoft WAV format (little endian). */
    return SF_FORMAT_WAV;

  if(!strcmp(str, "AIFF")) /* Apple/SGI AIFF format (big endian). */
    return SF_FORMAT_AIFF;

  if(!strcmp(str, "AU")) /* Sun/NeXT AU format (big endian). */
    return SF_FORMAT_AU;

  if(!strcmp(str, "RAW")) /* RAW PCM data. */
    return SF_FORMAT_RAW;

  if(!strcmp(str, "PAF")) /* Ensoniq PARIS file format. */
    return SF_FORMAT_PAF;

  if(!strcmp(str, "SVX")) /* Amiga IFF / SVX8 / SV16 format. */
    return SF_FORMAT_SVX;

  if(!strcmp(str, "NIST")) /* Sphere NIST format. */
    return SF_FORMAT_NIST;

  if(!strcmp(str, "VOC")) /* VOC files. */
    return SF_FORMAT_VOC;

  if(!strcmp(str, "IRCAM")) /* Berkeley/IRCAM/CARL */
    return SF_FORMAT_IRCAM;

  if(!strcmp(str, "W64")) /* Sonic Foundry's 64 bit RIFF/WAV */
    return SF_FORMAT_W64;

  if(!strcmp(str, "MAT4")) /* Matlab (tm) V4.2 / GNU Octave 2.0 */
    return SF_FORMAT_MAT4;

  if(!strcmp(str, "MAT5")) /* Matlab (tm) V5.0 / GNU Octave 2.1 */
    return SF_FORMAT_MAT5;

  if(!strcmp(str, "PVF")) /* Portable Voice Format */
    return SF_FORMAT_PVF;

  if(!strcmp(str, "XI")) /* Fasttracker 2 Extended Instrument */
    return SF_FORMAT_XI;

  if(!strcmp(str, "HTK")) /* HMM Tool Kit format */
    return SF_FORMAT_HTK;

  if(!strcmp(str, "SDS")) /* Midi Sample Dump Standard */
    return SF_FORMAT_SDS;

  if(!strcmp(str, "AVR")) /* Audio Visual Research */
    return SF_FORMAT_AVR;

  if(!strcmp(str, "WAVEX")) /* MS WAVE with WAVEFORMATEX */
    return SF_FORMAT_WAVEX;

  if(!strcmp(str, "SD2")) /* Sound Designer 2 */
    return SF_FORMAT_SD2;

  if(!strcmp(str, "FLAC")) /* FLAC lossless file format */
    return SF_FORMAT_FLAC;

  if(!strcmp(str, "CAF")) /* Core Audio File format */
    return SF_FORMAT_CAF;

  if(!strcmp(str, "WVE")) /* Psion WVE format */
    return SF_FORMAT_WVE;

  if(!strcmp(str, "OGG")) /* Xiph OGG container */
    return SF_FORMAT_OGG;

  if(!strcmp(str, "MPC2K")) /* Akai MPC 2000 sampler */
    return SF_FORMAT_MPC2K;

  if(!strcmp(str, "RF64")) /* RF64 WAV file */
    return SF_FORMAT_RF64;

  return -1;
}

static const char* sndfile_format_number2string(int format)
{
  format = format & SF_FORMAT_TYPEMASK;

  if(format == SF_FORMAT_WAV) /* Microsoft WAV format (little endian). */
    return "WAV";

  if(format == SF_FORMAT_AIFF) /* Apple/SGI AIFF format (big endian). */
    return "AIFF";

  if(format == SF_FORMAT_AU) /* Sun/NeXT AU format (big endian). */
    return "AU";

  if(format == SF_FORMAT_RAW) /* RAW PCM data. */
    return "RAW";

  if(format == SF_FORMAT_PAF) /* Ensoniq PARIS file format. */
    return "PAF";

  if(format == SF_FORMAT_SVX) /* Amiga IFF / SVX8 / SV16 format. */
    return "SVX";

  if(format == SF_FORMAT_NIST) /* Sphere NIST format. */
    return "NIST";

  if(format == SF_FORMAT_VOC) /* VOC files. */
    return "VOC";

  if(format == SF_FORMAT_IRCAM) /* Berkeley/IRCAM/CARL */
    return "IRCAM";

  if(format == SF_FORMAT_W64) /* Sonic Foundry's 64 bit RIFF/WAV */
    return "W64";

  if(format == SF_FORMAT_MAT4) /* Matlab (tm) V4.2 / GNU Octave 2.0 */
    return "MAT4";

  if(format == SF_FORMAT_MAT5) /* Matlab (tm) V5.0 / GNU Octave 2.1 */
    return "MAT5";

  if(format == SF_FORMAT_PVF) /* Portable Voice Format */
    return "PVF";

  if(format == SF_FORMAT_XI) /* Fasttracker 2 Extended Instrument */
    return "XI";

  if(format == SF_FORMAT_HTK) /* HMM Tool Kit format */
    return "HTK";

  if(format == SF_FORMAT_SDS) /* Midi Sample Dump Standard */
    return "SDS";

  if(format == SF_FORMAT_AVR) /* Audio Visual Research */
    return "AVR";

  if(format == SF_FORMAT_WAVEX) /* MS WAVE with WAVEFORMATEX */
    return "WAVEX";

  if(format == SF_FORMAT_SD2) /* Sound Designer 2 */
    return "SD2";

  if(format == SF_FORMAT_FLAC) /* FLAC lossless file format */
    return "FLAC";

  if(format == SF_FORMAT_CAF) /* Core Audio File format */
    return "CAF";

  if(format == SF_FORMAT_WVE) /* Psion WVE format */
    return "WVE";

  if(format == SF_FORMAT_OGG) /* Xiph OGG container */
    return "OGG";

  if(format == SF_FORMAT_MPC2K) /* Akai MPC 2000 sampler */
    return "MPC2K";

  if(format == SF_FORMAT_RF64) /* RF64 WAV file */
    return "RF64";

  return NULL;
}

static int sndfile_subformat_string2number(const char *str)
{
  if(!strcmp(str, "PCMS8")) /* Signed 8 bit data */
    return SF_FORMAT_PCM_S8;

  if(!strcmp(str, "PCM16")) /* Signed 16 bit data */
    return SF_FORMAT_PCM_16;

  if(!strcmp(str, "PCM24")) /* Signed 24 bit data */
    return SF_FORMAT_PCM_24;

  if(!strcmp(str, "PCM32")) /* Signed 32 bit data */
    return SF_FORMAT_PCM_32;

  if(!strcmp(str, "PCMU8")) /* Unsigned 8 bit data (WAV and RAW only) */
    return SF_FORMAT_PCM_U8;

  if(!strcmp(str, "FLOAT")) /* 32 bit float data */
    return SF_FORMAT_FLOAT;

  if(!strcmp(str, "DOUBLE")) /* 64 bit float data */
    return SF_FORMAT_DOUBLE;

  if(!strcmp(str, "ULAW")) /* U-Law encoded. */
    return SF_FORMAT_ULAW;

  if(!strcmp(str, "ALAW")) /* A-Law encoded. */
    return SF_FORMAT_ALAW;

  if(!strcmp(str, "IMAADPCM")) /* IMA ADPCM. */
    return SF_FORMAT_IMA_ADPCM;

  if(!strcmp(str, "MSADPCM")) /* Microsoft ADPCM. */
    return SF_FORMAT_MS_ADPCM;

  if(!strcmp(str, "GSM610")) /* GSM 6.10 encoding. */
    return SF_FORMAT_GSM610;

  if(!strcmp(str, "VOXADPCM")) /* Oki Dialogic ADPCM encoding. */
    return SF_FORMAT_VOX_ADPCM;

  if(!strcmp(str, "G72132")) /* 32kbs G721 ADPCM encoding. */
    return SF_FORMAT_G721_32;

  if(!strcmp(str, "G72324")) /* 24kbs G723 ADPCM encoding. */
    return SF_FORMAT_G723_24;

  if(!strcmp(str, "G72340")) /* 40kbs G723 ADPCM encoding. */
    return SF_FORMAT_G723_40;

  if(!strcmp(str, "DWVW12")) /* 12 bit Delta Width Variable Word encoding. */
    return SF_FORMAT_DWVW_12;

  if(!strcmp(str, "DWVW16")) /* 16 bit Delta Width Variable Word encoding. */
    return SF_FORMAT_DWVW_16;

  if(!strcmp(str, "DWVW24")) /* 24 bit Delta Width Variable Word encoding. */
    return SF_FORMAT_DWVW_24;

  if(!strcmp(str, "DWVWN")) /* N bit Delta Width Variable Word encoding. */
    return SF_FORMAT_DWVW_N;

  if(!strcmp(str, "DPCM8")) /* 8 bit differential PCM (XI only) */
    return SF_FORMAT_DPCM_8;

  if(!strcmp(str, "DPCM16")) /* 16 bit differential PCM (XI only) */
    return SF_FORMAT_DPCM_16;

  if(!strcmp(str, "VORBIS")) /* Xiph Vorbis encoding. */
    return SF_FORMAT_VORBIS;

  return -1;
}

static const char* sndfile_subformat_number2string(int format)
{
  format = format & SF_FORMAT_SUBMASK;

  if(format == SF_FORMAT_PCM_S8) /* Signed 8 bit data */
    return "PCMS8";

  if(format == SF_FORMAT_PCM_16) /* Signed 16 bit data */
    return "PCM16";

  if(format == SF_FORMAT_PCM_24) /* Signed 24 bit data */
    return "PCM24";

  if(format == SF_FORMAT_PCM_32) /* Signed 32 bit data */
    return "PCM32";

  if(format == SF_FORMAT_PCM_U8) /* Unsigned 8 bit data (WAV and RAW only) */
    return "PCMU8";

  if(format == SF_FORMAT_FLOAT) /* 32 bit float data */
    return "FLOAT";

  if(format == SF_FORMAT_DOUBLE) /* 64 bit float data */
    return "DOUBLE";

  if(format == SF_FORMAT_ULAW) /* U-Law encoded. */
    return "ULAW";

  if(format == SF_FORMAT_ALAW) /* A-Law encoded. */
    return "ALAW";

  if(format == SF_FORMAT_IMA_ADPCM) /* IMA ADPCM. */
    return "IMAADPCM";

  if(format == SF_FORMAT_MS_ADPCM) /* Microsoft ADPCM. */
    return "MSADPCM";

  if(format == SF_FORMAT_GSM610) /* GSM 6.10 encoding. */
    return "GSM610";

  if(format == SF_FORMAT_VOX_ADPCM) /* Oki Dialogic ADPCM encoding. */
    return "VOXADPCM";

  if(format == SF_FORMAT_G721_32) /* 32kbs G721 ADPCM encoding. */
    return "G72132";

  if(format == SF_FORMAT_G723_24) /* 24kbs G723 ADPCM encoding. */
    return "G72324";

  if(format == SF_FORMAT_G723_40) /* 40kbs G723 ADPCM encoding. */
    return "G72340";

  if(format == SF_FORMAT_DWVW_12) /* 12 bit Delta Width Variable Word encoding. */
    return "DWVW12";

  if(format == SF_FORMAT_DWVW_16) /* 16 bit Delta Width Variable Word encoding. */
    return "DWVW16";

  if(format == SF_FORMAT_DWVW_24) /* 24 bit Delta Width Variable Word encoding. */
    return "DWVW24";

  if(format == SF_FORMAT_DWVW_N) /* N bit Delta Width Variable Word encoding. */
    return "DWVWN";

  if(format == SF_FORMAT_DPCM_8) /* 8 bit differential PCM (XI only) */
    return "DPCM8";

  if(format == SF_FORMAT_DPCM_16) /* 16 bit differential PCM (XI only) */
    return "DPCM16";

  if(format == SF_FORMAT_VORBIS) /* Xiph Vorbis encoding. */
    return "VORBIS";

  return NULL;
}

static int sndfile_endian_string2number(const char *str)
{
  if(!strcmp(str, "FILE")) /* Default file endian-ness. */
    return SF_ENDIAN_FILE;

  if(!strcmp(str, "LITTLE")) /* Force little endian-ness. */
    return SF_ENDIAN_LITTLE;

  if(!strcmp(str, "BIG")) /* Force big endian-ness. */
    return SF_ENDIAN_BIG;

  if(!strcmp(str, "CPU")) /* Force CPU endian-ness. */
    return SF_ENDIAN_CPU;

  return -1;
}

static const char* sndfile_endian_number2string(int format)
{
  format = format & SF_FORMAT_ENDMASK;

  if(format == SF_ENDIAN_FILE) /* Default file endian-ness. */
    return "FILE";

  if(format == SF_ENDIAN_LITTLE) /* Force little endian-ness. */
    return "LITTLE";

  if(format == SF_ENDIAN_BIG) /* Force big endian-ness. */
    return "BIG";

  if(format == SF_ENDIAN_CPU) /* Force CPU endian-ness. */
    return "CPU";

  return NULL;
}

