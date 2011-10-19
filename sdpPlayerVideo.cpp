/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s): Marcin Balicki
  Created on: 2011-02-10

  (C) Copyright 2011 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include "sdpPlayerVideo.h"
#include <math.h>
#include <QMenu>
#include <QGridLayout>
#include <iostream>
#include <sstream>
#include <QFileDialog>
#include <cisstStereoVision/svlFilterVideoFileWriter.h>

#include <cisstOSAbstraction/osaGetTime.h>
#include <cisstStereoVision/svlFilterOutput.h>

CMN_IMPLEMENT_SERVICES(sdpPlayerVideo);

sdpPlayerVideo::sdpPlayerVideo(const std::string & name, double period):
    sdpPlayerBase(name,period),
    TimestampOverlay(0)
{
    // create the user interface
    // create the user interface
    ExWidget.setupUi(&Widget);
    VideoWidget = new svlFilterImageOpenGLQtWidget();

    QGridLayout *CentralLayout = new QGridLayout(&MainWindow);
    CentralLayout->setContentsMargins(0, 0, 0, 0);
    CentralLayout->setRowStretch(0, 1);
    CentralLayout->setColumnStretch(1, 1);

    CentralLayout->addWidget(VideoWidget, 0, 0, 1, 4);
    CentralLayout->addWidget(&Widget,1,1,1,1);

}


sdpPlayerVideo::~sdpPlayerVideo()
{
}


void sdpPlayerVideo::MakeQTConnections(void)
{
    QObject::connect(ExWidget.PlayButton, SIGNAL(clicked()),
                     this, SLOT( QSlotPlayClicked()) );

    QObject::connect(ExWidget.TimeSlider, SIGNAL(sliderMoved(int)),
                     this, SLOT( QSlotSeekSliderMoved(int)) );

    QObject::connect(ExWidget.SyncCheck, SIGNAL(clicked(bool)),
                     this, SLOT( QSlotSyncCheck(bool)) );

    QObject::connect(ExWidget.StopButton, SIGNAL(clicked()),
                     this, SLOT( QSlotStopClicked()) );

    QObject::connect(ExWidget.SetSaveStartButton, SIGNAL(clicked()),
                     this, SLOT( QSlotSetSaveStartClicked()) );

    QObject::connect(ExWidget.SetSaveEndButton, SIGNAL(clicked()),
                     this, SLOT( QSlotSetSaveEndClicked()) );

    QObject::connect(ExWidget.OpenFileButton, SIGNAL(clicked()),
                     this, SLOT( QSlotOpenFileClicked()) );

}


void sdpPlayerVideo::Configure(const std::string & CMN_UNUSED(filename))
{
    MakeQTConnections();


    //Widget.setWindowTitle(QString::fromStdString(GetName()));
    // Widget.show();
    MainWindow.setWindowTitle(QString::fromStdString(GetName()));
    MainWindow.resize(300,500);
    MainWindow.show();
    //    LoadData();
    //  UpdateLimits();

}


void sdpPlayerVideo::Startup(void)
{


}


void sdpPlayerVideo::Run(void)
{
    ProcessQueuedEvents();
    ProcessQueuedCommands();

    //update the model (load data) etc.
    if (State == PLAY) {

        double currentTime = TimeServer.GetAbsoluteTimeInSeconds();
        Time = currentTime - PlayStartTime.Timestamp() + PlayStartTime.Data;

        if (Time > PlayUntilTime) {
            Time = PlayUntilTime;
            State = STOP;
        }
        else {
            //Load and Prep current data
            //source.Play();
            CMN_LOG_CLASS_RUN_WARNING<<"pos: "<<Source.GetPositionAtTime(Time.Data)<<std::endl;
            //CMN_LOG_CLASS_RUN_WARNING<<"at T: "<<source.GetTimeAtPosition(source.GetPositionAtTime(Time.Data))<<std::endl;
            Source.SetPosition(Source.GetPositionAtTime(Time.Data));
            Source.Play();
        }
    }
    //make sure we are at the correct seek position.
    else if (State == SEEK) {
        //Load and Prep current data
        // CMN_LOG_CLASS_RUN_WARNING<<"pos: "<<source.GetPositionAtTime(Time.Data)<<std::endl;
        CMN_LOG_CLASS_RUN_WARNING<<"at T: "<<Source.GetTimeAtPosition(Source.GetPositionAtTime(Time.Data))<<std::endl;
        Source.SetPosition(Source.GetPositionAtTime(Time.Data));
        Source.Play();
    }

    else if (State == STOP) {
        //do Nothing
        Source.Pause();
        // CMN_LOG_CLASS_RUN_WARNING<<"pos: "<<source.GetPositionAtTime(Time.Data)<<std::endl;
        // CMN_LOG_CLASS_RUN_WARNING<<"at T: "<<source.GetTimeAtPosition(source.GetPositionAtTime(Time.Data))<<std::endl;
    }

    //now display updated data in the qt thread space.
    if (Widget.isVisible()) {
        emit QSignalUpdateQT();
    }
}


//in QT thread space
void sdpPlayerVideo::UpdateQT(void)
{
    if (State == PLAY) {
        //Display the last datasample before Time.
        ExWidget.TimeSlider->setValue((int)Time.Data);
    }
    //Make sure we are at the correct seek location.
    else if (State == STOP) {
        //Optional: Test if the data needs to be updated:
        ExWidget.TimeSlider->setValue((int)Time.Data);
    }

    else if (State == SEEK) {
        //Optional: Test if the data needs to be updated:
        ExWidget.TimeSlider->setValue((int)Time.Data);
    }

    ExWidget.TimeLabel->setText(QString::number(Time.Data,'f', 3));
}


void sdpPlayerVideo::Play(const mtsDouble & time)
{
    if (Sync) {
        CMN_LOG_CLASS_RUN_DEBUG << "Play " << PlayStartTime << std::endl;
        State = PLAY;
        PlayUntilTime = PlayerDataInfo.DataEnd();
        PlayStartTime = time;
    }
}


void sdpPlayerVideo::Stop(const mtsDouble & time)
{
    if (Sync) {
        CMN_LOG_CLASS_RUN_DEBUG << "Stop " << time << std::endl;
        PlayUntilTime = time;
    }
}


void sdpPlayerVideo::Seek(const mtsDouble & time)
{
    if (Sync) {
        CMN_LOG_CLASS_RUN_DEBUG << "Seek " << time << std::endl;

        State = SEEK;
        PlayUntilTime = PlayerDataInfo.DataEnd();
        Time = time;
    }
}


void sdpPlayerVideo::Save(const sdpSaveParameters & saveParameters)
{
    if (Sync) {
        CMN_LOG_CLASS_RUN_VERBOSE << "Save " << saveParameters << std::endl;

        svlStreamManager            stream(8);
        svlFilterSourceVideoFile    source(1);
        svlFilterVideoFileWriter    writer;

        if (Source.SetFilePath(FileName) != SVL_OK) {
            CMN_LOG_CLASS_RUN_ERROR << std::endl << "Wrong file name... " << std::endl;
            return;
        }

        source.SetTargetFrequency(1000.0); // as fast as possible
        source.SetLoop(false);
        source.SetRange(source.GetPositionAtTime(saveParameters.Start()),source.GetPositionAtTime(saveParameters.Start()));

        std::string date;
        osaGetDateTimeString(date);
        writer.SetFilePath(date +"_"+ FileName);

        //        // setup vi
        //        deo file writer
        //        if (dst_path.empty()) {
        //               if (writer.DialogOpenFile() != SVL_OK) {
        //                   cerr << " -!- No destination file has been selected." << endl;
        //                   return -1;
        //               }
        //               writer.GetFilePath(dst_path);
        //           }
        //           else {
        //               if (!loadcodec || writer.LoadCodec("codec.dat") != SVL_OK) {
        //                   writer.DialogCodec(dst_path);
        //               }
        //               writer.OpenFile(dst_path);
        //           }
        //           if (loadcodec) {
        //               writer.SaveCodec("codec.dat");
        //           }
        //           std::string encoder;
        //           writer.GetCodecName(encoder);


        // chain filters to pipeline
        svlFilterOutput* output = 0;

        stream.SetSourceFilter(&source);
        output = source.GetOutput();

        output->Connect(writer.GetInput());

        CMN_LOG_CLASS_RUN_VERBOSE << "Converting: '" << FileName << "' to '" << date+"_"+FileName << std::endl;

        // initialize stream
        if (stream.Initialize() != SVL_OK) {

            CMN_LOG_CLASS_RUN_ERROR << "Failed when converting: '" << FileName << "' to '" << date+"_"+FileName << std::endl;

        }

        // start stream
        if (stream.Play() != SVL_OK) {

            CMN_LOG_CLASS_RUN_ERROR << "Failed when converting: '" << FileName << "' to '" << date+"_"+FileName << std::endl;
        }

        do {
            CMN_LOG_CLASS_RUN_VERBOSE << " > Frames processed: " << source.GetFrameCounter() << "     \r";
        } while (stream.IsRunning() && stream.WaitForStop(0.5) == SVL_WAIT_TIMEOUT);

        CMN_LOG_CLASS_RUN_VERBOSE << " > Frames processed: " << source.GetFrameCounter() << "           " << std::endl;

        if (stream.GetStreamStatus() < 0) {
            // Some error
            CMN_LOG_CLASS_RUN_ERROR << " -!- Error occured during conversion." << std::endl;
        }
        else {
            // Success
             CMN_LOG_CLASS_RUN_VERBOSE << " > Conversion done." << std::endl;
        }

        // release pipeline
        stream.Release();
    }
}


void sdpPlayerVideo::Quit(void)
{
    CMN_LOG_CLASS_RUN_DEBUG << "Quit" << std::endl;
    this->Kill();
}


void sdpPlayerVideo::QSlotPlayClicked(void)
{
    mtsDouble playTime = Time; //this should be read from the state table!!!
    playTime.Timestamp() = TimeServer.GetAbsoluteTimeInSeconds();

    if (Sync) {
        PlayRequest(playTime);
    } else {
        //not quite thread safe, if there is mts play call this can be corrupt.
        State = PLAY;
        PlayUntilTime = PlayerDataInfo.DataEnd();
        PlayStartTime = playTime;
    }
}


void sdpPlayerVideo::QSlotSeekSliderMoved(int c)
{
    mtsDouble t = c;

    if (Sync) {
        SeekRequest(t);
    } else {
        State = SEEK;
        Time = t;
    }
    PlayUntilTime = PlayerDataInfo.DataEnd();
}


void sdpPlayerVideo::QSlotSyncCheck(bool checked)
{
    Sync = checked;
}


void sdpPlayerVideo::QSlotStopClicked(void)
{
    mtsDouble now = Time;

    if (Sync) {
        StopRequest(now);
    } else {
        PlayUntilTime = now;
    }
}


void sdpPlayerVideo::LoadData(void)
{

    QString fileName = QFileDialog::getOpenFileName(&Widget, tr("Select video file"),tr("./"),tr("Audio (*.cvi *.avi *.mpg *.mov *.mpeg)"));

    if (fileName.isEmpty()) {
        CMN_LOG_CLASS_RUN_WARNING<<"File not selected, no data to load"<<std::endl;
        return;
    }

    SetupPipeline(fileName.toStdString());

    vctInt2 range;

    range[0] = 0;
    range[1] = Source.GetLength();

    PlayerDataInfo.DataStart() = Source.GetTimeAtPosition(range[0]);
    PlayerDataInfo.DataEnd() = Source.GetTimeAtPosition(range[1]);

    if (Time < PlayerDataInfo.DataStart()) {
        Time = PlayerDataInfo.DataStart();
    }

    if (Time > PlayerDataInfo.DataEnd()) {
        Time = PlayerDataInfo.DataEnd();
    }

    //This is the standard.
    PlayUntilTime = PlayerDataInfo.DataEnd();

    UpdatePlayerInfo(PlayerDataInfo);
    UpdateLimits();
}


void sdpPlayerVideo::QSlotSetSaveStartClicked(void)
{
    ExWidget.SaveStartSpin->setValue(Time.Data);
}


void sdpPlayerVideo::QSlotSetSaveEndClicked(void)
{
    ExWidget.SaveEndSpin->setValue(Time.Data);
}


void sdpPlayerVideo::QSlotOpenFileClicked(void){

    LoadData();

}

void sdpPlayerVideo::UpdateLimits()
{
    ExWidget.TimeSlider->setRange((int)PlayerDataInfo.DataStart(), (int)PlayerDataInfo.DataEnd());

    ExWidget.TimeStartLabel->setText( QString::number(PlayerDataInfo.DataStart(),'f', 3));
    ExWidget.TimeEndLabel->setText( QString::number( PlayerDataInfo.DataEnd(),'f', 3));

    ExWidget.SaveStartSpin->setRange( PlayerDataInfo.DataStart(), PlayerDataInfo.DataEnd());
    ExWidget.SaveEndSpin->setRange( PlayerDataInfo.DataStart(), PlayerDataInfo.DataEnd());
    ExWidget.SaveEndSpin->setValue(PlayerDataInfo.DataEnd());
}


void sdpPlayerVideo::SetupPipeline(const std::string &filename)
{


    StreamManager.Release();
    Source.SetChannelCount(1);

    if (Source.SetFilePath(filename) != SVL_OK) {
        CMN_LOG_CLASS_RUN_ERROR << std::endl << "Wrong file name... " << std::endl;
        return;
    }

    FileName = filename;


    if (!TimestampOverlay) {

        TimestampOverlay = new svlOverlayTimestamp (0, true, VideoWidget, svlRect(4, 4, 134, 21),
                                                    15.0, svlRGB(255, 200, 200), svlRGB(32, 32, 32));
        Overlay.AddOverlay(*TimestampOverlay);

        // chain filters to pipeline
        StreamManager.SetSourceFilter(&Source); // chain filters to pipeline
        Source.GetOutput()->Connect(Overlay.GetInput());
        Overlay.GetOutput()->Connect(VideoWidget->GetInput());

    }
    StreamManager.Play();
    Source.Pause();

}

void sdpPlayerVideo::SetSynced(bool isSynced) {

    ExWidget.SyncCheck->setChecked(isSynced);
    Sync = isSynced;

}



