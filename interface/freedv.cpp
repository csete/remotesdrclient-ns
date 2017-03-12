#include <QDebug>
#include "freedv.h"


CFreedv::CFreedv(QObject *parent) : QObject(parent)
{
    fdv = 0;
    // TODO: text callback

    nin = freedv_nin(fdv);
    nout = 0;
    frames = 0;

    prev_frames = 0;
    prev_errors = 0;
}

CFreedv::~CFreedv()
{
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
    qDebug() << "FreeDV mode:" << mode_str;
}

int CFreedv::process(int num, short *demod_in, short *audio_out)
{
    int nout_tmp;
    short speech_out[FREEDV_NSAMPLES];

    // store incoming samples in local buffer
    input_buffer.append(demod_in, num);

    if (input_buffer.length() < nin)
        return 0;

    nout = 0;

    while (input_buffer.length() >= nin)
    {
        // process nin samples
        nout_tmp = freedv_rx(fdv, speech_out, input_buffer.data());
        nout += nout_tmp;
        frames++;
        output_buffer.append(speech_out, nout_tmp);

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
