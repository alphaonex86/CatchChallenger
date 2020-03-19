include(../libcatchchallenger/lib.pri)
include(../qt/tiled/tiled.pri)

QT       += gui network core widgets opengl xml

DEFINES += CATCHCHALLENGER_CLIENT

wasm: DEFINES += CATCHCHALLENGER_NOAUDIO
#android: DEFINES += CATCHCHALLENGER_NOAUDIO
# see the file ClientVariableAudio.h
#DEFINES += CATCHCHALLENGER_NOAUDIO
LIBS -= -lzstd
!contains(DEFINES, CATCHCHALLENGER_NOAUDIO) {
QT += multimedia
SOURCES += \
    $$PWD/../libogg/bitwise.c \
    $$PWD/../libogg/framing.c \
    $$PWD/../opusfile/info.c \
    $$PWD/../opusfile/internal.c \
    $$PWD/../opusfile/opusfile.c \
    $$PWD/../opusfile/stream.c \
    $$PWD/../qt/Audio.cpp \
    $$PWD/../qt/QInfiniteBuffer.cpp

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

SOURCES += \
    $$PWD/../libzstd/lib/common/pool.c \
    $$PWD/../libzstd/lib/common/zstd_common.c \
    $$PWD/../libzstd/lib/common/entropy_common.c \
    $$PWD/../libzstd/lib/common/fse_decompress.c \
    $$PWD/../libzstd/lib/common/threading.c \
    $$PWD/../libzstd/lib/common/error_private.c \
    $$PWD/../libzstd/lib/common/xxbhash.c \
    $$PWD/../libzstd/lib/compress/zstd_compress_sequences.c \
    $$PWD/../libzstd/lib/compress/huf_compress.c \
    $$PWD/../libzstd/lib/compress/zstd_compress.c \
    $$PWD/../libzstd/lib/compress/zstd_lazy.c \
    $$PWD/../libzstd/lib/compress/hist.c \
    $$PWD/../libzstd/lib/compress/zstd_fast.c \
    $$PWD/../libzstd/lib/compress/zstd_opt.c \
    $$PWD/../libzstd/lib/compress/zstd_ldm.c \
    $$PWD/../libzstd/lib/compress/zstdmt_compress.c \
    $$PWD/../libzstd/lib/compress/fse_compress.c \
    $$PWD/../libzstd/lib/compress/zstd_compress_literals.c \
    $$PWD/../libzstd/lib/compress/zstd_double_fast.c \
    $$PWD/../libzstd/lib/decompress/zstd_ddict.c \
    $$PWD/../libzstd/lib/decompress/huf_decompress.c \
    $$PWD/../libzstd/lib/decompress/zstd_decompress.c \
    $$PWD/../libzstd/lib/decompress/zstd_decompress_block.c \
    $$PWD/../libzstd/lib/dictBuilder/cover.c \
    $$PWD/../libzstd/lib/dictBuilder/divsufsort.c \
    $$PWD/../libzstd/lib/dictBuilder/fastcover.c \
    $$PWD/../libzstd/lib/dictBuilder/zdict.c

HEADERS  += \
    $$PWD/../libogg/ogg.h \
    $$PWD/../libogg/os_types.h \
    $$PWD/../opusfile/internal.h \
    $$PWD/../opusfile/opusfile.h \
    $$PWD/../qt/Audio.h \
    $$PWD/../qt/QInfiniteBuffer.h

HEADERS  += \
    $$PWD/../libopus/include/opus.h \
    $$PWD/../libopus/include/opus_multistream.h \
    $$PWD/../libopus/include/opus_types.h \
    $$PWD/../libopus/include/opus_custom.h \
    $$PWD/../libopus/include/opus_projection.h \
    $$PWD/../libopus/include/opus_defines.h \
    $$PWD/../libopus/silk/structs.h \
    $$PWD/../libopus/silk/define.h \
    $$PWD/../libopus/silk/Inlines.h \
    $$PWD/../libopus/silk/API.h \
    $$PWD/../libopus/silk/pitch_est_defines.h \
    $$PWD/../libopus/silk/typedef.h \
    $$PWD/../libopus/silk/float/structs_FLP.h \
    $$PWD/../libopus/silk/float/SigProc_FLP.h \
    $$PWD/../libopus/silk/float/main_FLP.h \
    $$PWD/../libopus/silk/PLC.h \
    $$PWD/../libopus/silk/errors.h \
    $$PWD/../libopus/silk/resampler_rom.h \
    $$PWD/../libopus/silk/resampler_structs.h \
    $$PWD/../libopus/silk/tables.h \
    $$PWD/../libopus/silk/resampler_private.h \
    $$PWD/../libopus/silk/MacroCount.h \
    $$PWD/../libopus/silk/SigProc_FIX.h \
    $$PWD/../libopus/silk/control.h \
    $$PWD/../libopus/silk/NSQ.h \
    $$PWD/../libopus/silk/main.h \
    $$PWD/../libopus/silk/tuning_parameters.h \
    $$PWD/../libopus/silk/macros.h \
    $$PWD/../libopus/celt/ecintrin.h \
    $$PWD/../libopus/celt/modes.h \
    $$PWD/../libopus/celt/celt.h \
    $$PWD/../libopus/celt/arch.h \
    $$PWD/../libopus/celt/vq.h \
    $$PWD/../libopus/celt/entenc.h \
    $$PWD/../libopus/celt/entcode.h \
    $$PWD/../libopus/celt/static_modes_float.h \
    $$PWD/../libopus/celt/entdec.h \
    $$PWD/../libopus/celt/celt_lpc.h \
    $$PWD/../libopus/celt/float_cast.h \
    $$PWD/../libopus/celt/rate.h \
    $$PWD/../libopus/celt/bands.h \
    $$PWD/../libopus/celt/cwrs.h \
    $$PWD/../libopus/celt/fixed_generic.h \
    $$PWD/../libopus/celt/_kiss_fft_guts.h \
    $$PWD/../libopus/celt/pitch.h \
    $$PWD/../libopus/celt/mfrngcod.h \
    $$PWD/../libopus/celt/mdct.h \
    $$PWD/../libopus/celt/static_modes_fixed.h \
    $$PWD/../libopus/celt/cpu_support.h \
    $$PWD/../libopus/celt/kiss_fft.h \
    $$PWD/../libopus/celt/stack_alloc.h \
    $$PWD/../libopus/celt/laplace.h \
    $$PWD/../libopus/celt/mathops.h \
    $$PWD/../libopus/celt/quant_bands.h \
    $$PWD/../libopus/celt/os_support.h \
    $$PWD/../libopus/src/tansig_table.h \
    $$PWD/../libopus/src/opus_private.h \
    $$PWD/../libopus/src/mlp.h \
    $$PWD/../libopus/src/analysis.h \
    $$PWD/../libopus/src/mapping_matrix.h \
    $$PWD/../libopus/win32/config.h

HEADERS  += \
    $$PWD/../libzstd/lib/zstd.h \
    $$PWD/../libzstd/lib/common/compiler.h \
    $$PWD/../libzstd/lib/common/threading.h \
    $$PWD/../libzstd/lib/common/xxhash.h \
    $$PWD/../libzstd/lib/common/error_private.h \
    $$PWD/../libzstd/lib/common/bitstream.h \
    $$PWD/../libzstd/lib/common/fse.h \
    $$PWD/../libzstd/lib/common/mem.h \
    $$PWD/../libzstd/lib/common/huf.h \
    $$PWD/../libzstd/lib/common/zstd_internal.h \
    $$PWD/../libzstd/lib/common/pool.h \
    $$PWD/../libzstd/lib/common/cpu.h \
    $$PWD/../libzstd/lib/common/zstd_errors.h \
    $$PWD/../libzstd/lib/compress/zstd_fast.h \
    $$PWD/../libzstd/lib/compress/zstd_lazy.h \
    $$PWD/../libzstd/lib/compress/zstd_compress_internal.h \
    $$PWD/../libzstd/lib/compress/zstdmt_compress.h \
    $$PWD/../libzstd/lib/compress/zstd_cwksp.h \
    $$PWD/../libzstd/lib/compress/zstd_opt.h \
    $$PWD/../libzstd/lib/compress/hist.h \
    $$PWD/../libzstd/lib/compress/zstd_compress_sequences.h \
    $$PWD/../libzstd/lib/compress/zstd_ldm.h \
    $$PWD/../libzstd/lib/compress/zstd_double_fast.h \
    $$PWD/../libzstd/lib/compress/zstd_compress_literals.h \
    $$PWD/../libzstd/lib/decompress/zstd_ddict.h \
    $$PWD/../libzstd/lib/decompress/zstd_decompress_block.h \
    $$PWD/../libzstd/lib/decompress/zstd_decompress_internal.h \
    $$PWD/../libzstd/lib/dictBuilder/cover.h \
    $$PWD/../libzstd/lib/dictBuilder/zdict.h \
    $$PWD/../libzstd/lib/dictBuilder/divsufsort.h

INCLUDEPATH += $$PWD/../libopus/include/
INCLUDEPATH += $$PWD/../libzstd/contrib/linux-kernel/include/
#Opus requires one of VAR_ARRAYS, USE_ALLOCA, or NONTHREADSAFE_PSEUDOSTACK be defined to select the temporary allocation mode.
DEFINES += USE_ALLOCA OPUS_BUILD ZSTD_STATIC_LINKING_ONLY
}
wasm: {
    DEFINES += NOTCPSOCKET NOSINGLEPLAYER NOTHREADS
}
else
{
#    DEFINES += NOWEBSOCKET
}
android: {
    INCLUDEPATH += /opt/android-sdk/ndk-r19c/platforms/android-21/arch-arm64/usr/include/
    INCLUDEPATH += /opt/android-sdk/ndk-r19c/platforms/android-21/arch-arm/usr/include/
    LIBS += -L/opt/qt/5.12.4/android_arm64_v8a/lib/
    LIBS += -L/opt/qt/5.12.4/android_armv7/lib/
    QT += androidextras
    QMAKE_CFLAGS += -g
    QMAKE_CXXFLAGS += -g
    DISTFILES += $$PWD/../android-sources/AndroidManifest.xml
}
!contains(DEFINES, NOWEBSOCKET) {
    QT += websockets
}

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2b.cpp \
    $$PWD/../../general/base/tinyXML2/tinyxml2c.cpp \
    $$PWD/LanguagesSelect.cpp \
    $$PWD/CCTitle.cpp \
    $$PWD/ConnexionManager.cpp \
    $$PWD/LoadingScreen.cpp \
    $$PWD/CCWidget.cpp \
    $$PWD/CustomButton.cpp \
    $$PWD/MainScreen.cpp \
    $$PWD/Map_client.cpp \
    $$PWD/ScreenTransition.cpp \
    $$PWD/Multi.cpp \
    $$PWD/OptionsDialog.cpp \
    $$PWD/CCBackground.cpp \
    $$PWD/CCprogressbar.cpp \
    $$PWD/../qt/Api_client_real.cpp \
    $$PWD/../qt/Api_client_real_base.cpp \
    $$PWD/../qt/Api_client_real_main.cpp \
    $$PWD/../qt/Api_client_real_sub.cpp \
    $$PWD/../qt/Api_client_virtual.cpp \
    $$PWD/../qt/Api_protocol_Qt.cpp \
    $$PWD/../qt/QtDatapackChecksum.cpp \
    $$PWD/../qt/QtDatapackClientLoader.cpp \
    $$PWD/../qt/QZstdDecodeThread.cpp \
    $$PWD/../qt/InternetUpdater.cpp \
    $$PWD/../qt/FeedNews.cpp \
    $$PWD/../qt/ConnectedSocket.cpp \
    $$PWD/../qt/GameLoader.cpp \
    $$PWD/../tarcompressed/TarDecode.cpp \
    $$PWD/../tarcompressed/ZstdDecode.cpp \
    $$PWD/../qt/fight/interface/ClientFightEngine.cpp \
    $$PWD/../qt/fight/interface/DatapackClientLoaderFight.cpp \
    $$PWD/../qt/crafting/interface/DatapackClientLoaderCrafting.cpp \
    $$PWD/../qt/Ultimate.cpp \
    $$PWD/../qt/GameLoaderThread.cpp \
    $$PWD/../qt/ExtraSocket.cpp \
    $$PWD/../qt/Settings.cpp \
    $$PWD/ScreenInput.cpp \
    $$PWD/AudioGL.cpp

HEADERS  += \
    $$PWD/../../general/base/tinyXML2/tinyxml2.h \
    $$PWD/LanguagesSelect.h \
    $$PWD/CCprogressbar.h \
    $$PWD/ClientStructures.h \
    $$PWD/CCTitle.h \
    $$PWD/ConnexionManager.h \
    $$PWD/CCWidget.h \
    $$PWD/CustomButton.h \
    $$PWD/LoadingScreen.h \
    $$PWD/DisplayStructures.h \
    $$PWD/MainScreen.h \
    $$PWD/Map_client.h \
    $$PWD/OptionsDialog.h \
    $$PWD/ScreenTransition.h \
    $$PWD/Multi.h \
    $$PWD/CCBackground.h \
    $$PWD/../qt/Api_client_real.h \
    $$PWD/../qt/Api_client_virtual.h \
    $$PWD/../qt/Api_protocol_Qt.h \
    $$PWD/../qt/QtDatapackChecksum.h \
    $$PWD/../qt/QtDatapackClientLoader.h \
    $$PWD/../qt/QZstdDecodeThread.h \
    $$PWD/../qt/InternetUpdater.h \
    $$PWD/../qt/FeedNews.h \
    $$PWD/../qt/ConnectedSocket.h \
    $$PWD/../qt/GameLoader.h \
    $$PWD/../tarcompressed/TarDecode.h \
    $$PWD/../tarcompressed/ZstdDecode.h \
    $$PWD/../qt/fight/interface/ClientFightEngine.h \
    $$PWD/../qt/Ultimate.h \
    $$PWD/../qt/GameLoaderThread.h \
    $$PWD/../qt/ExtraSocket.h \
    $$PWD/../qt/Settings.h \
    $$PWD/ScreenInput.h \
    $$PWD/AudioGL.h

RESOURCES += $$PWD/../resources/client-resources-multi.qrc
DEFINES += CATCHCHALLENGER_MULTI
DEFINES += CATCHCHALLENGER_CLASS_CLIENT

#commented to workaround to compil under wine
win32:RC_FILE += $$PWD/../resources/resources-windows.rc
ICON = $$PWD/../resources/client.icns
macx:INCLUDEPATH += /Users/user/Desktop/VLC.app/Contents/MacOS/include/
macx:LIBS += -L/Users/user/Desktop/VLC.app/Contents/MacOS/lib/

RESOURCES += $$PWD/../resources/client-resources.qrc \
    $$PWD/../qt/crafting/resources/client-resources-plant.qrc \
    $$PWD/../qt/fight/resources/client-resources-fight.qrc

TRANSLATIONS    = $$PWD/../resources/languages/en/translation.ts \
    $$PWD/../languages/fr/translation.ts

#define CATCHCHALLENGER_SOLO
contains(DEFINES, CATCHCHALLENGER_SOLO) {
include(../../server/catchchallenger-server-qt.pri)
QT       += sql
SOURCES += $$PWD/../qt/solo/InternalServer.cpp \
    $$PWD/../qt/solo/SoloWindow.cpp
HEADERS  += $$PWD/../qt/solo/InternalServer.h \
    $$PWD/../qt/solo/SoloWindow.h
}

DISTFILES += \
    $$PWD/client.pri
