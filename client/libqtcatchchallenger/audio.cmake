# Audio path — vendored libogg + libopus + libopusfile as static libs.
# Mirrors libogg.pri / libopus.pri / libopusfile.pri (instead of the
# upstream CMakeLists.txt of each vendored project, which add install
# rules + cross-package find_package(Ogg) we'd have to satisfy in the
# in-tree build). Keep upstream sources untouched per CLAUDE.md.

set(_libogg_root ${CMAKE_CURRENT_SOURCE_DIR}/libogg)
add_library(catchchallenger_libogg STATIC
    ${_libogg_root}/src/bitwise.c
    ${_libogg_root}/src/framing.c
)
target_include_directories(catchchallenger_libogg PUBLIC ${_libogg_root}/include)

set(_libopus_root ${CMAKE_CURRENT_SOURCE_DIR}/libopus)
# Pulled from libopus.pri SOURCES list (CELT + SILK common + SILK float
# + opus core + opus float analysis). 134 .c files — listed verbatim.
set(_libopus_celt
    bands.c celt.c celt_encoder.c celt_decoder.c cwrs.c entcode.c
    entdec.c entenc.c kiss_fft.c laplace.c mathops.c mdct.c modes.c
    pitch.c celt_lpc.c quant_bands.c rate.c vq.c
)
set(_libopus_silk
    CNG.c code_signs.c init_decoder.c decode_core.c decode_frame.c
    decode_parameters.c decode_indices.c decode_pulses.c decoder_set_fs.c
    dec_API.c enc_API.c encode_indices.c encode_pulses.c gain_quant.c
    interpolate.c LP_variable_cutoff.c NLSF_decode.c NSQ.c NSQ_del_dec.c
    PLC.c shell_coder.c tables_gain.c tables_LTP.c tables_NLSF_CB_NB_MB.c
    tables_NLSF_CB_WB.c tables_other.c tables_pitch_lag.c
    tables_pulses_per_block.c VAD.c control_audio_bandwidth.c
    quant_LTP_gains.c VQ_WMat_EC.c HP_variable_cutoff.c NLSF_encode.c
    NLSF_VQ.c NLSF_unpack.c NLSF_del_dec_quant.c process_NLSFs.c
    stereo_LR_to_MS.c stereo_MS_to_LR.c check_control_input.c
    control_SNR.c init_encoder.c control_codec.c A2NLSF.c
    ana_filt_bank_1.c biquad_alt.c bwexpander_32.c bwexpander.c
    debug.c decode_pitch.c inner_prod_aligned.c lin2log.c log2lin.c
    LPC_analysis_filter.c LPC_inv_pred_gain.c table_LSF_cos.c NLSF2A.c
    NLSF_stabilize.c NLSF_VQ_weights_laroia.c pitch_est_tables.c
    resampler.c resampler_down2_3.c resampler_down2.c
    resampler_private_AR2.c resampler_private_down_FIR.c
    resampler_private_IIR_FIR.c resampler_private_up2_HQ.c resampler_rom.c
    sigm_Q15.c sort.c sum_sqr_shift.c stereo_decode_pred.c
    stereo_encode_pred.c stereo_find_predictor.c stereo_quant_pred.c
    LPC_fit.c
)
set(_libopus_silk_float
    apply_sine_window_FLP.c corrMatrix_FLP.c encode_frame_FLP.c
    find_LPC_FLP.c find_LTP_FLP.c find_pitch_lags_FLP.c
    find_pred_coefs_FLP.c LPC_analysis_filter_FLP.c
    LTP_analysis_filter_FLP.c LTP_scale_ctrl_FLP.c
    noise_shape_analysis_FLP.c process_gains_FLP.c
    regularize_correlations_FLP.c residual_energy_FLP.c
    warped_autocorrelation_FLP.c wrappers_FLP.c autocorrelation_FLP.c
    burg_modified_FLP.c bwexpander_FLP.c energy_FLP.c
    inner_product_FLP.c k2a_FLP.c LPC_inv_pred_gain_FLP.c
    pitch_analysis_core_FLP.c scale_copy_vector_FLP.c
    scale_vector_FLP.c schur_FLP.c sort_FLP.c
)
set(_libopus_src
    opus.c opus_decoder.c opus_encoder.c extensions.c
    opus_multistream.c opus_multistream_encoder.c
    opus_multistream_decoder.c repacketizer.c
    opus_projection_encoder.c opus_projection_decoder.c
    mapping_matrix.c
    analysis.c mlp.c mlp_data.c
)
list(TRANSFORM _libopus_celt PREPEND ${_libopus_root}/celt/)
list(TRANSFORM _libopus_silk PREPEND ${_libopus_root}/silk/)
list(TRANSFORM _libopus_silk_float PREPEND ${_libopus_root}/silk/float/)
list(TRANSFORM _libopus_src PREPEND ${_libopus_root}/src/)

add_library(catchchallenger_libopus STATIC
    ${_libopus_celt} ${_libopus_silk} ${_libopus_silk_float} ${_libopus_src}
)
target_include_directories(catchchallenger_libopus PUBLIC
    ${_libopus_root}
    ${_libopus_root}/include
    ${_libopus_root}/celt
    ${_libopus_root}/silk
    ${_libopus_root}/silk/float
    ${_libopus_root}/src
)
target_compile_definitions(catchchallenger_libopus PRIVATE
    OPUS_BUILD HAVE_CONFIG_H FLOAT_APPROX
    HAVE_LRINT HAVE_LRINTF HAVE_STDINT_H
    VAR_ARRAYS
)

set(_libopusfile_root ${CMAKE_CURRENT_SOURCE_DIR}/libopusfile)
add_library(catchchallenger_libopusfile STATIC
    ${_libopusfile_root}/src/info.c
    ${_libopusfile_root}/src/internal.c
    ${_libopusfile_root}/src/opusfile.c
    ${_libopusfile_root}/src/stream.c
)
target_include_directories(catchchallenger_libopusfile PUBLIC
    ${_libopusfile_root}/include
    ${_libopusfile_root}/src
)
target_compile_definitions(catchchallenger_libopusfile PRIVATE OP_DISABLE_HTTP)
target_link_libraries(catchchallenger_libopusfile PUBLIC
    catchchallenger_libopus
    catchchallenger_libogg
)
