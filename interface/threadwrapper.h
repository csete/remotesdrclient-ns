/////////////////////////////////////////////////////////////////////
// threadwrapper.h  CThreadWrapper class.
//This base class is used to derive thread based objects that will have their
//own event loop running in this worker thread.  The init() function is called by
// the worker thread when started to setup all signal-slot connections
//The ThreadExit() function is called by the thread after CleanupThread() is
//called.  This can be used in cases where the worker thread must delete its own resources
// as is the case for network objects.
// History:
//	2013-10-02  Initial creation MSW
/////////////////////////////////////////////////////////////////////
//==========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2013 Moe Wheatley. All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are
//permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this list of
//	  conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice, this list
//	  of conditions and the following disclaimer in the documentation and/or other materials
//	  provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
//WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//The views and conclusions contained in the software and documentation are those of the
//authors and should not be interpreted as representing official policies, either expressed
//or implied, of Moe Wheatley.
//==========================================================================================
#ifndef THREADWRAPPER_H
#define THREADWRAPPER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QDebug>

class CThreadWrapper : public QObject
{
	Q_OBJECT
public:
	CThreadWrapper()
	{
		m_pThread = new QThread(this);
		this->moveToThread(m_pThread);
		connect(m_pThread, SIGNAL(started()), this,SLOT(ThreadInit()) );
		connect(this, SIGNAL(ThreadExitSignal() ), this,SLOT( ThreadExit() ) );
		m_pThread->start();
	}
	virtual ~CThreadWrapper()
	{
		m_pThread->exit();
		m_pThread->wait();
		delete m_pThread;
	}
	QMutex m_Mutex;
	void CleanupThread(){emit ThreadExitSignal(); m_pThread->wait(10);}

signals:
	void ThreadExitSignal();

public slots:
	virtual void ThreadInit() = 0;	//derived class must override. called by new thread when started
	virtual void ThreadExit() = 0;

protected:
	QThread* m_pThread;
};

#endif // THREADWRAPPER_H
