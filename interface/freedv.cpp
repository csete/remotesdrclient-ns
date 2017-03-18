#include <QDebug>
#include "freedv.h"


CFreedv::CFreedv(QObject *parent) : QObject(parent)
{
    fdv = 0;
    // TODO: text callback

    num_speech_samples = 0;
    nin = 0;
    nout = 0;
    frames = 0;
    prev_frames = 0;
    prev_errors = 0;
}

CFreedv::~CFreedv()
{
    if (fdv)
        freedv_close(fdv);
}

void CFreedv::set_mode(const QString &mode_str)
{
    int fdv_mode;

    if (fdv)
    {
        freedv_close(fdv);
        fdv = 0;
    }

    // NB: 2400A and 2400B are I/Q modes so we can't use them here
    if (mode_str.contains("1600"))
        fdv_mode = FREEDV_MODE_1600;
    else if (mode_str.contains("700B"))
        fdv_mode = FREEDV_MODE_700B;
    else if (mode_str.contains("700C"))
        fdv_mode = FREEDV_MODE_700C;
    else if (mode_str.contains("800XA"))
        fdv_mode = FREEDV_MODE_800XA;
    else
        return;

    fdv = freedv_open(fdv_mode);
    if (!fdv)
        return;

    qDebug() << "FreeDV mode:" << mode_str;

    // update buffer sizes
    nin = freedv_nin(fdv);
    num_speech_samples = freedv_get_n_speech_samples(fdv);
    output_buffer.resize(4 * num_speech_samples);
    input_buffer.resize(4 * freedv_get_n_max_modem_samples(fdv));
}

int CFreedv::process(int num, short *demod_in, short *audio_out)
{
    int nout_tmp;
    //short speech_out[FREEDV_NSAMPLES];
    QVarLengthArray<short, FREEDV_NSAMPLES> speech_out(num_speech_samples);

    // store incoming samples in local buffer
    input_buffer.append(demod_in, num);

    if (input_buffer.length() < nin)
        return 0;

    nout = 0;

    while (input_buffer.length() >= nin)
    {
        // process nin samples
        //nout_tmp = freedv_rx(fdv, speech_out, input_buffer.data());
        nout_tmp = freedv_rx(fdv, speech_out.data(), input_buffer.data());
        nout += nout_tmp;
        frames++;
        output_buffer.append(speech_out.data(), nout_tmp);

        // remove processed data from local buffer
        input_buffer.erase(input_buffer.begin(), input_buffer.begin() + nin);

        nin = freedv_nin(fdv);
    }

    // copy contents of out buffer to return var and clear buffer
    memcpy(audio_out, output_buffer.data(), output_buffer.length() * sizeof(short));
    output_buffer.clear();

    return nout; // output samples provided in audio_out
}


float   CFreedv::demod_ber()
{
    return 0.f;
/*    float bits = codec2_bits_per_frame((CODEC2 *)fdv->codec2) * (frames - prev_frames);
    float errors = fdv->total_bit_errors - prev_errors;

    prev_frames = frames;
    prev_errors = fdv->total_bit_errors;

    return errors / bits;*/
}
