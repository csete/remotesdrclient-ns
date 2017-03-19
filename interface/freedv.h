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
    int     get_sync() const
    {
        return sync;
    }
    float   get_snr() const
    {
        return snr;
    }
    float   demod_ber();

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
