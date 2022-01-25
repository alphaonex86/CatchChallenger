DEFINES += CATCHCHALLENGER_CLIENT

wasm: DEFINES += CATCHCHALLENGER_NOAUDIO
# see the file ClientVariableAudio.h
#DEFINES += CATCHCHALLENGER_NOAUDIO
!contains(DEFINES, CATCHCHALLENGER_NOAUDIO) {
QT += multimedia
linux:LIBS += -lopus
macx:LIBS += -lopus
win32:LIBS += -lopus
wasm:LIBS += -Lopus
}

SOURCES += \
    $$PWD/Api_client_real_base.cpp \
    $$PWD/Api_client_real.cpp \
    $$PWD/Api_client_real_main.cpp \
    $$PWD/Api_client_real_sub.cpp \
    $$PWD/Api_client_virtual.cpp \
    $$PWD/Api_protocol_Qt.cpp \
    $$PWD/ClientFightEngine.cpp \
    $$PWD/QtDatapackClientLoader.cpp \
    $$PWD/QtDatapackClientLoaderThread.cpp \
    $$PWD/QtDatapackChecksum.cpp \
    $$PWD/QZstdDecodeThread.cpp \
    $$PWD/Language.cpp \
    $$PWD/Settings.cpp \
    $$PWD/ConnectedSocket.cpp

DEFINES += CATCHCHALLENGERLIB

!contains(DEFINES, CATCHCHALLENGER_NOAUDIO) {
QT += multimedia
SOURCES += \
    $$PWD/../libogg/bitwise.c \
    $$PWD/../libogg/framing.c \
    $$PWD/../opusfile/info.c \
    $$PWD/../opusfile/internal.c \
    $$PWD/../opusfile/opusfile.c \
    $$PWD/../opusfile/stream.c \
    $$PWD/Audio.cpp \
    $$PWD/QInfiniteBuffer.cpp

SOURCES += \
    $$PWD/../libopus/silk/ana_filt_bank_1.c \
    $$PWD/../libopus/silk/inner_prod_aligned.c \
    $$PWD/../libopus/silk/CNG.c \
    $$PWD/../libopus/silk/process_NLSFs.c \
    $$PWD/../libopus/silk/init_decoder.c \
    $$PWD/../libopus/silk/tables_pulses_per_block.c \
    $$PWD/../libopus/silk/NLSF_VQ_weights_laroia.c \
    $$PWD/../libopus/silk/encode_pulses.c \
    $$PWD/../libopus/silk/resampler_rom.c \
    $$PWD/../libopus/silk/interpolate.c \
    $$PWD/../libopus/silk/NLSF_unpack.c \
    $$PWD/../libopus/silk/bwexpander.c \
    $$PWD/../libopus/silk/tables_LTP.c \
    $$PWD/../libopus/silk/NLSF_decode.c \
    $$PWD/../libopus/silk/LPC_fit.c \
    $$PWD/../libopus/silk/stereo_decode_pred.c \
    $$PWD/../libopus/silk/tables_gain.c \
    $$PWD/../libopus/silk/tables_NLSF_CB_NB_MB.c \
    $$PWD/../libopus/silk/stereo_quant_pred.c \
    $$PWD/../libopus/silk/NLSF_encode.c \
    $$PWD/../libopus/silk/sigm_Q15.c \
    $$PWD/../libopus/silk/resampler.c \
    $$PWD/../libopus/silk/float/encode_frame_FLP.c \
    $$PWD/../libopus/silk/float/pitch_analysis_core_FLP.c \
    $$PWD/../libopus/silk/float/residual_energy_FLP.c \
    $$PWD/../libopus/silk/float/corrMatrix_FLP.c \
    $$PWD/../libopus/silk/float/find_LTP_FLP.c \
    $$PWD/../libopus/silk/float/LTP_analysis_filter_FLP.c \
    $$PWD/../libopus/silk/float/find_pred_coefs_FLP.c \
    $$PWD/../libopus/silk/float/wrappers_FLP.c \
    $$PWD/../libopus/silk/float/schur_FLP.c \
    $$PWD/../libopus/silk/float/burg_modified_FLP.c \
    $$PWD/../libopus/silk/float/bwexpander_FLP.c \
    $$PWD/../libopus/silk/float/find_pitch_lags_FLP.c \
    $$PWD/../libopus/silk/float/autocorrelation_FLP.c \
    $$PWD/../libopus/silk/float/energy_FLP.c \
    $$PWD/../libopus/silk/float/warped_autocorrelation_FLP.c \
    $$PWD/../libopus/silk/float/regularize_correlations_FLP.c \
    $$PWD/../libopus/silk/float/LPC_analysis_filter_FLP.c \
    $$PWD/../libopus/silk/float/LTP_scale_ctrl_FLP.c \
    $$PWD/../libopus/silk/float/process_gains_FLP.c \
    $$PWD/../libopus/silk/float/k2a_FLP.c \
    $$PWD/../libopus/silk/float/noise_shape_analysis_FLP.c \
    $$PWD/../libopus/silk/float/apply_sine_window_FLP.c \
    $$PWD/../libopus/silk/float/find_LPC_FLP.c \
    $$PWD/../libopus/silk/float/scale_copy_vector_FLP.c \
    $$PWD/../libopus/silk/float/inner_product_FLP.c \
    $$PWD/../libopus/silk/float/scale_vector_FLP.c \
    $$PWD/../libopus/silk/float/sort_FLP.c \
    $$PWD/../libopus/silk/float/LPC_inv_pred_gain_FLP.c \
    $$PWD/../libopus/silk/NSQ_del_dec.c \
    $$PWD/../libopus/silk/NLSF_stabilize.c \
    $$PWD/../libopus/silk/stereo_encode_pred.c \
    $$PWD/../libopus/silk/check_control_input.c \
    $$PWD/../libopus/silk/sort.c \
    $$PWD/../libopus/silk/NSQ.c \
    $$PWD/../libopus/silk/dec_API.c \
    $$PWD/../libopus/silk/HP_variable_cutoff.c \
    $$PWD/../libopus/silk/VQ_WMat_EC.c \
    $$PWD/../libopus/silk/control_SNR.c \
    $$PWD/../libopus/silk/stereo_LR_to_MS.c \
    $$PWD/../libopus/silk/PLC.c \
    $$PWD/../libopus/silk/shell_coder.c \
    $$PWD/../libopus/silk/resampler_private_AR2.c \
    $$PWD/../libopus/silk/decode_core.c \
    $$PWD/../libopus/silk/resampler_down2_3.c \
    $$PWD/../libopus/silk/log2lin.c \
    $$PWD/../libopus/silk/NLSF_del_dec_quant.c \
    $$PWD/../libopus/silk/tables_other.c \
    $$PWD/../libopus/silk/init_encoder.c \
    $$PWD/../libopus/silk/tables_NLSF_CB_WB.c \
    $$PWD/../libopus/silk/decode_pitch.c \
    $$PWD/../libopus/silk/lin2log.c \
    $$PWD/../libopus/silk/NLSF_VQ.c \
    $$PWD/../libopus/silk/decode_indices.c \
    $$PWD/../libopus/silk/quant_LTP_gains.c \
    $$PWD/../libopus/silk/decode_pulses.c \
    $$PWD/../libopus/silk/LPC_inv_pred_gain.c \
    $$PWD/../libopus/silk/bwexpander_32.c \
    $$PWD/../libopus/silk/resampler_private_IIR_FIR.c \
    $$PWD/../libopus/silk/LP_variable_cutoff.c \
    $$PWD/../libopus/silk/A2NLSF.c \
    $$PWD/../libopus/silk/NLSF2A.c \
    $$PWD/../libopus/silk/pitch_est_tables.c \
    $$PWD/../libopus/silk/decode_frame.c \
    $$PWD/../libopus/silk/gain_quant.c \
    $$PWD/../libopus/silk/biquad_alt.c \
    $$PWD/../libopus/silk/control_audio_bandwidth.c \
    $$PWD/../libopus/silk/control_codec.c \
    $$PWD/../libopus/silk/decode_parameters.c \
    $$PWD/../libopus/silk/table_LSF_cos.c \
    $$PWD/../libopus/silk/encode_indices.c \
    $$PWD/../libopus/silk/code_signs.c \
    $$PWD/../libopus/silk/sum_sqr_shift.c \
    $$PWD/../libopus/silk/tables_pitch_lag.c \
    $$PWD/../libopus/silk/stereo_find_predictor.c \
    $$PWD/../libopus/silk/LPC_analysis_filter.c \
    $$PWD/../libopus/silk/resampler_private_up2_HQ.c \
    $$PWD/../libopus/silk/enc_API.c \
    $$PWD/../libopus/silk/stereo_MS_to_LR.c \
    $$PWD/../libopus/silk/resampler_down2.c \
    $$PWD/../libopus/silk/VAD.c \
    $$PWD/../libopus/silk/decoder_set_fs.c \
    $$PWD/../libopus/silk/resampler_private_down_FIR.c \
    $$PWD/../libopus/celt/celt_lpc.c \
    $$PWD/../libopus/celt/celt_encoder.c \
    $$PWD/../libopus/celt/pitch.c \
    $$PWD/../libopus/celt/celt.c \
    $$PWD/../libopus/celt/quant_bands.c \
    $$PWD/../libopus/celt/celt_decoder.c \
    $$PWD/../libopus/celt/kiss_fft.c \
    $$PWD/../libopus/celt/mathops.c \
    $$PWD/../libopus/celt/vq.c \
    $$PWD/../libopus/celt/laplace.c \
    $$PWD/../libopus/celt/bands.c \
    $$PWD/../libopus/celt/entenc.c \
    $$PWD/../libopus/celt/entdec.c \
    $$PWD/../libopus/celt/modes.c \
    $$PWD/../libopus/celt/mdct.c \
    $$PWD/../libopus/celt/rate.c \
    $$PWD/../libopus/celt/cwrs.c \
    $$PWD/../libopus/celt/entcode.c \
    $$PWD/../libopus/src/opus_multistream_decoder.c \
    $$PWD/../libopus/src/opus_multistream.c \
    $$PWD/../libopus/src/mlp_data.c \
    $$PWD/../libopus/src/opus_projection_decoder.c \
    $$PWD/../libopus/src/opus_decoder.c \
    $$PWD/../libopus/src/mapping_matrix.c \
    $$PWD/../libopus/src/mlp.c \
    $$PWD/../libopus/src/opus_projection_encoder.c \
    $$PWD/../libopus/src/analysis.c \
    $$PWD/../libopus/src/opus_compare.c \
    $$PWD/../libopus/src/opus_encoder.c \
    $$PWD/../libopus/src/repacketizer.c \
    $$PWD/../libopus/src/opus_multistream_encoder.c \
    $$PWD/../libopus/src/opus.c
}
