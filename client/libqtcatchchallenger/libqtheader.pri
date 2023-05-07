HEADERS  += \
    $$PWD/Api_client_real.hpp \
    $$PWD/Api_client_virtual.hpp \
    $$PWD/Api_protocol_Qt.hpp \
    $$PWD/QtDatapackClientLoader.hpp \
    $$PWD/QtDatapackClientLoaderThread.hpp \
    $$PWD/QtDatapackChecksum.hpp \
    $$PWD/QZstdDecodeThread.hpp \
    $$PWD/QInfiniteBuffer.hpp \
    $$PWD/Audio.hpp \
    $$PWD/Language.hpp \
    $$PWD/DisplayStructures.hpp \
    $$PWD/PlatformMacro.hpp \
    $$PWD/Settings.hpp \
    $$PWD/ClientVariableAudio.hpp \
    $$PWD/FeedNews.hpp \
    $$PWD/InternetUpdater.hpp \
    $$PWD/Ultimate.hpp \
    $$PWD/LanBroadcastWatcher.hpp \
    $$PWD/LocalListener.hpp \
    $$PWD/ExtraSocket.hpp \
    $$PWD/ConnectedSocket.hpp

!contains(DEFINES, CATCHCHALLENGER_NOAUDIO) {
QT += multimedia

HEADERS  += \
    $$PWD/../libogg/ogg.h \
    $$PWD/../libogg/os_types.h \
    $$PWD/../opusfile/internal.h \
    $$PWD/../opusfile/opusfile.h \
    $$PWD/Audio.hpp \
    $$PWD/QInfiniteBuffer.hpp

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

    INCLUDEPATH += $$PWD/../libopus/include/
    #Opus requires one of VAR_ARRAYS, USE_ALLOCA, or NONTHREADSAFE_PSEUDOSTACK be defined to select the temporary allocation mode.
    DEFINES += USE_ALLOCA OPUS_BUILD
}


