#ifndef CFREEDV_H
#define CFREEDV_H

#include <codec2/freedv_api.h>
#include <QObject>
#include <QString>
#include <QVarLengthArray>

class CFreedv : public QObject
{
    Q_OBJECT
public:
    explicit CFreedv(QObject *parent = 0);
    ~CFreedv();

    void    set_mode(const QString &mode_str);
    int     process(int num, short *demod_in, short *audio_out);
    int     demod_locked() const
    {
        return 0;//fdv->fdmdv_stats.sync;
    }
    float   demod_snr() const
    {
        return 0;//fdv->fdmdv_stats.snr_est;
    }
    float   demod_ber();

signals:

public slots:

private:
#define FREEDV_NSAMPLES 512     // from earlier API; bufsizes will be updated in set_mode()
    QVarLengthArray<short, FREEDV_NSAMPLES> output_buffer;
    QVarLengthArray<short, FREEDV_NSAMPLES> input_buffer;
    struct freedv  *fdv;
    int             num_speech_samples; // mode dependent
    int             nin, nout, frames;
    int             prev_frames, prev_errors;
};

#endif // CFREEDV_H
