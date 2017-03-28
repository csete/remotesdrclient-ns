#ifndef CFREEDV_H
#define CFREEDV_H

#include <freedv_api.h>
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
    void    get_snr(int * _sync, float * _snr) const
    {
        *_sync = sync;
        *_snr = snr;
    }
    bool    is_active(void) const
    {
        return fdv != 0;
    }

signals:

public slots:

private:
    QVarLengthArray<short>  output_buffer;
    QVarLengthArray<short>  input_buffer;
    struct freedv  *fdv;
    int             num_speech_samples; // mode dependent
    int             num_max_modem_samples;

    // statistics
    int             sync;
    float           snr;
};

#endif
