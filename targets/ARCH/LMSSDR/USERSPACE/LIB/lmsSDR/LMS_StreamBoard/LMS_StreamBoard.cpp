#include "LMS_StreamBoard.h"
#include "lmsComms.h"
#include "lms7002_defines.h"
#include <assert.h>
#include <iostream>

/** @brief Configures Stream board FPGA clocks
    @param serPort Communications port to send data
    @param fOutTx_MHz transmitter frequency in MHz
    @param fOutRx_MHz receiver frequency in MHz
    @param phaseShift_deg IQ phase shift in degrees
    @return 0-success, other-failure
*/
LMS_StreamBoard::Status LMS_StreamBoard::ConfigurePLL(LMScomms *serPort, const float fOutTx_MHz, const float fOutRx_MHz, const float phaseShift_deg)
{
    assert(serPort != nullptr);
    if (serPort == NULL)
        return FAILURE;
    if (serPort->IsOpen() == false)
        return FAILURE;
    const float vcoLimits_MHz[2] = { 600, 1300 };
    int M, C;
    const short bufSize = 64;

    float fOut_MHz = fOutTx_MHz;
    float coef = 0.8*vcoLimits_MHz[1] / fOut_MHz;
    M = C = (int)coef;
    int chigh = (((int)coef) / 2) + ((int)(coef) % 2);
    int clow = ((int)coef) / 2;

    LMScomms::GenericPacket pkt;
    pkt.cmd = CMD_BRDSPI_WR;
    
    if (fOut_MHz*M > vcoLimits_MHz[0] && fOut_MHz*M < vcoLimits_MHz[1])
    {   
        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x0F);
        pkt.outBuffer.push_back(0x15); //c4-c2_bypassed
        pkt.outBuffer.push_back(0x01 | ((M % 2 != 0) ? 0x08 : 0x00) | ((C % 2 != 0) ? 0x20 : 0x00)); //N_bypassed

        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x08);
        pkt.outBuffer.push_back(1); //N_high_cnt
        pkt.outBuffer.push_back(1);//N_low_cnt
        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x09);
        pkt.outBuffer.push_back(chigh); //M_high_cnt
        pkt.outBuffer.push_back(clow);	 //M_low_cnt
        for (int i = 0; i <= 1; ++i)
        {
            pkt.outBuffer.push_back(0x00);
            pkt.outBuffer.push_back(0x0A + i);
            pkt.outBuffer.push_back(chigh); //cX_high_cnt
            pkt.outBuffer.push_back(clow);	 //cX_low_cnt
        }

        float Fstep_us = 1 / (8 * fOutTx_MHz*C);
        float Fstep_deg = (360 * Fstep_us) / (1 / fOutTx_MHz);
        short nSteps = phaseShift_deg / Fstep_deg;
        unsigned short reg2 = 0x0400 | (nSteps & 0x3FF);
        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x02);
        pkt.outBuffer.push_back((reg2 >> 8));
        pkt.outBuffer.push_back(reg2); //phase

        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x03);
        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x01);

        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x03);
        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x00);

        reg2 = reg2 | 0x800;
        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x02);
        pkt.outBuffer.push_back((reg2 >> 8));
        pkt.outBuffer.push_back(reg2);

        if(serPort->TransferPacket(pkt) != LMScomms::TRANSFER_SUCCESS || pkt.status != STATUS_COMPLETED_CMD)
            return FAILURE;
    }
    else
        return FAILURE;

    fOut_MHz = fOutRx_MHz;
    coef = 0.8*vcoLimits_MHz[1] / fOut_MHz;
    M = C = (int)coef;
    chigh = (((int)coef) / 2) + ((int)(coef) % 2);
    clow = ((int)coef) / 2;
    if (fOut_MHz*M > vcoLimits_MHz[0] && fOut_MHz*M < vcoLimits_MHz[1])
    {
        short index = 0;
        pkt.outBuffer.clear();
        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x0F);
        pkt.outBuffer.push_back(0x15); //c4-c2_bypassed
        pkt.outBuffer.push_back(0x41 | ((M % 2 != 0) ? 0x08 : 0x00) | ((C % 2 != 0) ? 0x20 : 0x00)); //N_bypassed, c1 bypassed

        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x08);
        pkt.outBuffer.push_back(1); //N_high_cnt
        pkt.outBuffer.push_back(1);//N_low_cnt
        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x09);
        pkt.outBuffer.push_back(chigh); //M_high_cnt
        pkt.outBuffer.push_back(clow);	 //M_low_cnt
        for (int i = 0; i <= 1; ++i)
        {
            pkt.outBuffer.push_back(0x00);
            pkt.outBuffer.push_back(0x0A + i);
            pkt.outBuffer.push_back(chigh); //cX_high_cnt
            pkt.outBuffer.push_back(clow);	 //cX_low_cnt
        }

        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x03);
        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x02);

        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x03);
        pkt.outBuffer.push_back(0x00);
        pkt.outBuffer.push_back(0x00);

        if (serPort->TransferPacket(pkt) != LMScomms::TRANSFER_SUCCESS || pkt.status != STATUS_COMPLETED_CMD)
            return FAILURE;
    }
    else
        return FAILURE;
    return SUCCESS;
}

/** @brief Captures IQ samples from Stream board, this is blocking function, it blocks until desired frames count is captured
    @param isamples destination array for I samples, must be big enough to contain samplesCount
    @param qsamples destination array for Q samples, must be big enough to contain samplesCount
    @param framesCount number of IQ frames to capture
    @param frameStart frame start indicator 0 or 1
    @return 0-success, other-failure
*/
LMS_StreamBoard::Status LMS_StreamBoard::CaptureIQSamples(LMScomms *dataPort, int16_t *isamples, int16_t *qsamples, const uint32_t framesCount, const bool frameStart)
{
    assert(dataPort != nullptr);
    if (dataPort == NULL)
        return FAILURE;
    if (dataPort->IsOpen() == false)
        return FAILURE;

    int16_t sample_value;
    const uint32_t bufSize = framesCount * 2 * sizeof(uint16_t);
    char *buffer = new char[bufSize];
    if (buffer == 0)
    {
#ifndef NDEBUG
        std::cout << "Failed to allocate memory for samples buffer" << std::endl;
#endif
        return FAILURE;
    }
    memset(buffer, 0, bufSize);

    LMScomms::GenericPacket pkt;
    pkt.cmd = CMD_BRDSPI_RD;
    pkt.outBuffer.push_back(0x00);
    pkt.outBuffer.push_back(0x05);
    dataPort->TransferPacket(pkt);
    if (pkt.status != STATUS_COMPLETED_CMD)
        return FAILURE;

    uint16_t regVal = (pkt.inBuffer[2] * 256) + pkt.inBuffer[3];
    pkt.cmd = CMD_BRDSPI_WR;
    pkt.outBuffer.clear();
    pkt.outBuffer.push_back(0x00);
    pkt.outBuffer.push_back(0x05);
    pkt.outBuffer.push_back(0);
    pkt.outBuffer.push_back(regVal | 0x4);    
    dataPort->TransferPacket(pkt);
    if (pkt.status != STATUS_COMPLETED_CMD)
        return FAILURE;

    int bytesReceived = 0;
    for(int i = 0; i<3; ++i)
        bytesReceived = dataPort->ReadStream(buffer, bufSize, 5000);
    if (bytesReceived > 0)
    {
        bool iqSelect = false;
        int16_t frameCounter = 0;        
        for (uint32_t b = 0; b < bufSize;)
        {
            sample_value = buffer[b++] & 0xFF;
            sample_value |= (buffer[b++] & 0x0F) << 8;
            sample_value = sample_value << 4; //shift left then right to fill sign bits
            sample_value = sample_value >> 4;
            if (iqSelect == false)
                isamples[frameCounter] = sample_value;
            else
                qsamples[frameCounter] = sample_value;
            frameCounter += iqSelect;
            iqSelect = !iqSelect;
        }        
    }
    pkt.cmd = CMD_BRDSPI_RD;
    pkt.outBuffer.clear();
    pkt.outBuffer.push_back(0x00);
    pkt.outBuffer.push_back(0x01);
    pkt.outBuffer.push_back(0x00);
    pkt.outBuffer.push_back(0x05);    
    dataPort->TransferPacket(pkt);

    regVal = (pkt.inBuffer[2] * 256) + pkt.inBuffer[3];
    pkt.cmd = CMD_BRDSPI_WR;
    pkt.outBuffer.clear();
    pkt.outBuffer.push_back(0x00);
    pkt.outBuffer.push_back(0x05);
    pkt.outBuffer.push_back(0);
    pkt.outBuffer.push_back(regVal & ~0x4);    
    dataPort->TransferPacket(pkt);

    delete[] buffer;
    return SUCCESS;
}

/** @brief Blocking operation to upload IQ samples to Stream board RAM
    @param serPort port to use for communication
    @param isamples I channel samples
    @param qsamples Q channel samples
    @param framesCount number of samples in arrays
    @return 0-success, other-failure
*/
LMS_StreamBoard::Status LMS_StreamBoard::UploadIQSamples(LMScomms* serPort, int16_t *isamples, int16_t *qsamples, const uint32_t framesCount)
{
    int bufferSize = framesCount * 2;
    uint16_t *buffer = new uint16_t[bufferSize];
    memset(buffer, 0, bufferSize*sizeof(uint16_t));
    int bufPos = 0;
    for (unsigned i = 0; i<framesCount; ++i)
    {   
        buffer[bufPos] = (isamples[i] & 0xFFF);
        buffer[bufPos + 1] = (qsamples[i] & 0xFFF) | 0x1000;
        bufPos += 2;
    }
    const long outLen = bufPos * 2;
    int packetSize = 65536;
    int sent = 0;
    bool success = true;

    LMScomms::GenericPacket pkt;
    pkt.cmd = CMD_BRDSPI_RD;
    pkt.outBuffer.push_back(0x00);
    pkt.outBuffer.push_back(0x05);

    serPort->TransferPacket(pkt);
    pkt.cmd = CMD_BRDSPI_WR;
    pkt.outBuffer.clear();
    pkt.outBuffer.push_back(0x00);
    pkt.outBuffer.push_back(0x05);
    pkt.outBuffer.push_back(pkt.inBuffer[2]);
    pkt.outBuffer.push_back(pkt.inBuffer[3] & ~0x7);
    serPort->TransferPacket(pkt);

    while (sent<outLen)
    {
        char *outBuf = (char*)buffer;
        const long toSendBytes = outLen - sent > packetSize ? packetSize : outLen - sent;
        long toSend = toSendBytes;
        int context = serPort->BeginDataSending(&outBuf[sent], toSend);
        if (serPort->WaitForSending(context, 5000) == false)
        {
            success = false;
            serPort->FinishDataSending(&outBuf[sent], toSend, context);
            break;
        }
        sent += serPort->FinishDataSending(&outBuf[sent], toSend, context);
    }
    
    pkt.cmd = CMD_BRDSPI_RD;
    pkt.outBuffer.push_back(0x00);
    pkt.outBuffer.push_back(0x05);

    serPort->TransferPacket(pkt);
    pkt.cmd = CMD_BRDSPI_WR;
    pkt.outBuffer.clear();
    pkt.outBuffer.push_back(0x00);
    pkt.outBuffer.push_back(0x05);
    pkt.outBuffer.push_back(pkt.inBuffer[2]);
    pkt.outBuffer.push_back(pkt.inBuffer[3] | 0x3);
    serPort->TransferPacket(pkt);
    return success ? SUCCESS : FAILURE;
}

LMS_StreamBoard::LMS_StreamBoard(LMScomms* dataPort)
{
    mRxFrameStart.store(true);
    mDataPort = dataPort;
    mRxFIFO = new LMS_StreamBoard_FIFO<SamplesPacket>(1024*4);
    mTxFIFO = new LMS_StreamBoard_FIFO<SamplesPacket>(1024*4);
    mStreamRunning.store(false);
    mTxCyclicRunning.store(false);
}

LMS_StreamBoard::~LMS_StreamBoard()
{
    StopReceiving();
    delete mRxFIFO;
    delete mTxFIFO;
}

/** @brief Crates threads for packets receiving, processing and transmitting
*/
LMS_StreamBoard::Status LMS_StreamBoard::StartReceiving(unsigned int fftSize)
{
    if (mStreamRunning.load() == true)
        return FAILURE;
    if (mDataPort->IsOpen() == false)
        return FAILURE;
    mRxFIFO->reset();    
    stopRx.store(false);
    threadRx = std::thread(ReceivePackets, this);
    mStreamRunning.store(true);
    return SUCCESS;
}

/** @brief Stops receiving, processing and transmitting threads
*/
LMS_StreamBoard::Status LMS_StreamBoard::StopReceiving()
{
    if (mStreamRunning.load() == false)
        return FAILURE;
    stopTx.store(true);
    stopRx.store(true);    
    threadRx.join();
    mStreamRunning.store(false);
    return SUCCESS;
}

/** @brief Function dedicated for receiving data samples from board
*/
void LMS_StreamBoard::ReceivePackets(LMS_StreamBoard* pthis)
{
    SamplesPacket pkt;
    int samplesCollected = 0;
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = chrono::high_resolution_clock::now();

    const int buffer_size = 65536;// 4096;
    const int buffers_count = 16; // must be power of 2
    const int buffers_count_mask = buffers_count - 1;
    int handles[buffers_count];
    memset(handles, 0, sizeof(int)*buffers_count);
    char *buffers = NULL;
    buffers = new char[buffers_count*buffer_size];
    if (buffers == 0)
    {
        printf("error allocating buffers\n");
        return;
    }
    memset(buffers, 0, buffers_count*buffer_size);

    //USB FIFO reset
    LMScomms::GenericPacket ctrPkt;
    ctrPkt.cmd = CMD_USB_FIFO_RST;
    ctrPkt.outBuffer.push_back(0x01);
    pthis->mDataPort->TransferPacket(ctrPkt);
    ctrPkt.outBuffer[0] = 0x00;
    pthis->mDataPort->TransferPacket(ctrPkt);

    uint16_t regVal = pthis->SPI_read(0x0005);
    pthis->SPI_write(0x0005, regVal | 0x4);

    for (int i = 0; i<buffers_count; ++i)
        handles[i] = pthis->mDataPort->BeginDataReading(&buffers[i*buffer_size], buffer_size);

    int bi = 0;
    int packetsReceived = 0;
    unsigned long BytesReceived = 0;
    int m_bufferFailures = 0;    
    short sample;

    bool frameStart = pthis->mRxFrameStart.load();
    while (pthis->stopRx.load() == false)
    {
        if (pthis->mDataPort->WaitForReading(handles[bi], 1000) == false)
            ++m_bufferFailures;

        long bytesToRead = buffer_size;
        long bytesReceived = pthis->mDataPort->FinishDataReading(&buffers[bi*buffer_size], bytesToRead, handles[bi]);
        if (bytesReceived > 0)
        {
            ++packetsReceived;
            BytesReceived += bytesReceived;
            char* bufStart = &buffers[bi*buffer_size];
            
            for (int p = 0; p < bytesReceived; p+=2)
            {
                if (samplesCollected == 0) //find frame start
                {
                    int frameStartOffset = FindFrameStart(&bufStart[p], bytesReceived-p, frameStart);
                    if (frameStartOffset < 0) 
                        break; //frame start was not found, move on to next buffer
                    p += frameStartOffset;
                }
                sample = (bufStart[p+1] & 0x0F);
                sample = sample << 8;
                sample |= (bufStart[p] & 0xFF);
                sample = sample << 4;
                sample = sample >> 4;
                pkt.iqdata[samplesCollected] = sample;
                ++samplesCollected;
                if (pkt.samplesCount == samplesCollected)
                {
                    samplesCollected = 0;
                    if (pthis->mRxFIFO->push_back(pkt, 200) == false)
                        ++m_bufferFailures;
                }
            }
        }
        else
        {
            ++m_bufferFailures;
        }

        t2 = chrono::high_resolution_clock::now();
        long timePeriod = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        if (timePeriod >= 1000)
        {
            // periodically update frame start, user might change it during operation
            frameStart = pthis->mRxFrameStart.load();
            float m_dataRate = 1000.0*BytesReceived / timePeriod;
            t1 = t2;
            BytesReceived = 0;
            LMS_StreamBoard_FIFO<SamplesPacket>::Status rxstats = pthis->mRxFIFO->GetStatus();

            pthis->mRxDataRate.store(m_dataRate);
            pthis->mRxFIFOfilled.store(100.0*rxstats.filledElements / rxstats.maxElements);
#ifndef NDEBUG
            printf("Rx: %.1f%% \t rate: %.0f kB/s failures:%i\n", 100.0*rxstats.filledElements / rxstats.maxElements, m_dataRate / 1000.0, m_bufferFailures);
#endif
            m_bufferFailures = 0;
        }

        // Re-submit this request to keep the queue full
        memset(&buffers[bi*buffer_size], 0, buffer_size);
        handles[bi] = pthis->mDataPort->BeginDataReading(&buffers[bi*buffer_size], buffer_size);
        bi = (bi + 1) & buffers_count_mask;
    }
    pthis->mDataPort->AbortReading();
    for (int j = 0; j<buffers_count; j++)
    {
        long bytesToRead = buffer_size;
        pthis->mDataPort->WaitForReading(handles[j], 1000);
        pthis->mDataPort->FinishDataReading(&buffers[j*buffer_size], bytesToRead, handles[j]);
    }

    regVal = pthis->SPI_read(0x0005);
    pthis->SPI_write(0x0005, regVal & ~0x4);

    delete[] buffers;
#ifndef NDEBUG
    printf("Rx finished\n");
#endif
}

/** @brief Function dedicated for transmitting samples to board
*/
void LMS_StreamBoard::TransmitPackets(LMS_StreamBoard* pthis)
{
    const int packetsToBatch = 16;
    const int buffer_size = sizeof(SamplesPacket)*packetsToBatch;
    const int buffers_count = 16; // must be power of 2
    const int buffers_count_mask = buffers_count - 1;
    int handles[buffers_count];
    memset(handles, 0, sizeof(int)*buffers_count);
    char *buffers = NULL;
    buffers = new char[buffers_count*buffer_size];
    if (buffers == 0)
    {
        printf("error allocating buffers\n");
    }
    memset(buffers, 0, buffers_count*buffer_size);
    bool *bufferUsed = new bool[buffers_count];
    memset(bufferUsed, 0, sizeof(bool)*buffers_count);

    int bi = 0; //buffer index

    SamplesPacket* pkt = new SamplesPacket[packetsToBatch];
    int m_bufferFailures = 0;
    long bytesSent = 0;
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = chrono::high_resolution_clock::now();
    long totalBytesSent = 0;

    unsigned long outputCounter = 0;

    while (pthis->stopTx.load() == false)
    {
        for (int i = 0; i < packetsToBatch; ++i)
        {
            if (pthis->mTxFIFO->pop_front(&pkt[i], 200) == false)
            {
                printf("Error popping from TX\n");
                if (pthis->stopTx.load() == false)
                    break;
                continue;
            }
        }
        //wait for desired slot buffer to be transferred
        if (bufferUsed[bi])
        {
            if (pthis->mDataPort->WaitForSending(handles[bi], 1000) == false)
            {
                ++m_bufferFailures;
            }
            // Must always call FinishDataXfer to release memory of contexts[i]
            long bytesToSend = buffer_size;
            bytesSent = pthis->mDataPort->FinishDataSending(&buffers[bi*buffer_size], bytesToSend, handles[bi]);
            if (bytesSent > 0)
                totalBytesSent += bytesSent;
            else
                ++m_bufferFailures;
            bufferUsed[bi] = false;
        }

        memcpy(&buffers[bi*buffer_size], &pkt[0], sizeof(SamplesPacket)*packetsToBatch);
        handles[bi] = pthis->mDataPort->BeginDataSending(&buffers[bi*buffer_size], buffer_size);
        bufferUsed[bi] = true;

        t2 = chrono::high_resolution_clock::now();
        long timePeriod = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        if (timePeriod >= 1000)
        {
            float m_dataRate = 1000.0*totalBytesSent / timePeriod;
            t1 = t2;
            totalBytesSent = 0;
#ifndef NDEBUG
            cout << "Tx rate: " << m_dataRate / 1000 << " kB/s\t failures: " << m_bufferFailures << endl;
#endif
            LMS_StreamBoard_FIFO<SamplesPacket>::Status txstats = pthis->mTxFIFO->GetStatus();
            pthis->mTxDataRate.store(m_dataRate);
            pthis->mTxFIFOfilled.store(100.0*txstats.filledElements / txstats.maxElements);
            m_bufferFailures = 0;
        }
        bi = (bi + 1) & buffers_count_mask;
    }

    // Wait for all the queued requests to be cancelled
    pthis->mDataPort->AbortSending();
    for (int j = 0; j<buffers_count; j++)
    {
        long bytesToSend = buffer_size;
        if (bufferUsed[bi])
        {
            pthis->mDataPort->WaitForSending(handles[j], 1000);
            pthis->mDataPort->FinishDataSending(&buffers[j*buffer_size], bytesToSend, handles[j]);
        }
    }

    delete[] buffers;
    delete[] bufferUsed;
    delete[] pkt;
#ifndef NDEBUG
    printf("Tx finished\n");
#endif
}

/** @brief Returns current data state for user interface
*/
LMS_StreamBoard::DataToGUI LMS_StreamBoard::GetIncomingData()
{
    std::unique_lock<std::mutex> lck(mLockIncomingPacket);
    return mIncomingPacket;
}

/** @brief Returns data rate info and Tx Rx FIFO fill percentage
*/
LMS_StreamBoard::ProgressStats LMS_StreamBoard::GetStats()
{
    ProgressStats stats;
    stats.RxRate_Bps = mRxDataRate.load();
    stats.TxRate_Bps = mTxDataRate.load();
    stats.RxFIFOfilled = mRxFIFOfilled.load();
    stats.TxFIFOfilled = mTxFIFOfilled.load();
    return stats;
}

/** @brief Helper function to write board spi regiters
    @param address spi address
    @param data register value
*/
LMS_StreamBoard::Status LMS_StreamBoard::SPI_write(uint16_t address, uint16_t data)
{
    assert(mDataPort != nullptr);
    LMScomms::GenericPacket ctrPkt;
    ctrPkt.cmd = CMD_BRDSPI_WR;
    ctrPkt.outBuffer.push_back((address >> 8) & 0xFF);
    ctrPkt.outBuffer.push_back(address & 0xFF);
    ctrPkt.outBuffer.push_back((data >> 8) & 0xFF);
    ctrPkt.outBuffer.push_back(data & 0xFF);
    mDataPort->TransferPacket(ctrPkt);
    return ctrPkt.status == 1 ? SUCCESS : FAILURE;
}

/** @brief Helper function to read board spi registers
    @param address spi address
    @return register value
*/
uint16_t LMS_StreamBoard::SPI_read(uint16_t address)
{
    assert(mDataPort != nullptr);
    LMScomms::GenericPacket ctrPkt;
    ctrPkt.cmd = CMD_BRDSPI_RD;
    ctrPkt.outBuffer.push_back((address >> 8) & 0xFF);
    ctrPkt.outBuffer.push_back(address & 0xFF);
    mDataPort->TransferPacket(ctrPkt);
    if (ctrPkt.status == STATUS_COMPLETED_CMD && ctrPkt.inBuffer.size() >= 4)
        return ctrPkt.inBuffer[2] * 256 + ctrPkt.inBuffer[3];
    else
        return 0;
}

/** @brief Changes which frame start to look for when receiving data
*/
void LMS_StreamBoard::SetRxFrameStart(const bool startValue)
{
    mRxFrameStart.store(startValue);
}

/** @brief Searches for frame start index in given buffer
    @return frameStart index in buffer, -1 if frame start was not found
    Frame start indicator is 13th bit inside each I and Q sample
*/
int LMS_StreamBoard::FindFrameStart(const char* buffer, const int bufLen, const bool frameStart)
{
    int startIndex = -1;
    for (int i = 0; i < bufLen; i+=2)
        if ((buffer[i+1] & 0x10)>0 == frameStart)
        {
            startIndex = i;
            break;
        }
    return startIndex;
}

/** @brief Starts a thread for continuous cyclic transmitting of given samples
    @param isamples I channel samples
    @param qsamples Q channel samples
    @param framesCount number of samples in given arrays
    @return 0:success, other:failure
*/
LMS_StreamBoard::Status LMS_StreamBoard::StartCyclicTransmitting(const int16_t* isamples, const int16_t* qsamples, uint32_t framesCount)
{   
    if (mDataPort->IsOpen() == false)
        return FAILURE;

    stopTxCyclic.store(false);
    threadTxCyclic = std::thread([](LMS_StreamBoard* pthis)
    {
        const int buffer_size = 65536;
        const int buffers_count = 16; // must be power of 2
        const int buffers_count_mask = buffers_count - 1;
        int handles[buffers_count];
        memset(handles, 0, sizeof(int)*buffers_count);
        char *buffers = NULL;
        buffers = new char[buffers_count*buffer_size];
        if (buffers == 0)
        {
            printf("error allocating buffers\n");
            return 0;
        }
        memset(buffers, 0, buffers_count*buffer_size);

        //timers for data rate calculation
        auto t1 = chrono::high_resolution_clock::now();
        auto t2 = chrono::high_resolution_clock::now();

        int bi = 0; //buffer index

        //setup output data
        int dataIndex = 0;
        for (int i = 0; i < buffers_count; ++i)
        {
            for (int j = 0; j < buffer_size; ++j)
            {
                buffers[i*buffer_size + j] = pthis->mCyclicTransmittingSourceData[dataIndex];
                ++dataIndex;
                if (dataIndex > pthis->mCyclicTransmittingSourceData.size())
                    dataIndex = 0;
            }
        }

        for (int i = 0; i < buffers_count; ++i)
            handles[i] = pthis->mDataPort->BeginDataSending(&buffers[i*buffer_size], buffer_size);

        int m_bufferFailures = 0;
        int bytesSent = 0;
        int totalBytesSent = 0;
        int sleepTime = 200;
        while (pthis->stopTxCyclic.load() != true)
        {
            if (pthis->mDataPort->WaitForSending(handles[bi], 1000) == false)
            {
                ++m_bufferFailures;
            }
            long bytesToSend = buffer_size;
            bytesSent = pthis->mDataPort->FinishDataSending(&buffers[bi*buffer_size], bytesToSend, handles[bi]);
            if (bytesSent > 0)
                totalBytesSent += bytesSent;
            else
            {
                ++m_bufferFailures;
            }

            t2 = chrono::high_resolution_clock::now();
            long timePeriod = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
            if (timePeriod >= 1000)
            {
                pthis->mTxDataRate.store(1000.0*totalBytesSent / timePeriod);
                t1 = t2;
                totalBytesSent = 0;
#ifndef NDEBUG
                printf("Upload rate: %f\t failures:%li\n", 1000.0*totalBytesSent / timePeriod, m_bufferFailures);
#endif
                m_bufferFailures = 0;
            }

            //fill up next buffer		
            for (int j = 0; j < buffer_size; ++j)
            {
                buffers[bi*buffer_size + j] = pthis->mCyclicTransmittingSourceData[dataIndex];
                ++dataIndex;
                if (dataIndex >= pthis->mCyclicTransmittingSourceData.size())
                    dataIndex = 0;
            }

            // Re-submit this request to keep the queue full
            handles[bi] = pthis->mDataPort->BeginDataSending(&buffers[bi*buffer_size], buffer_size);
            bi = (bi + 1) & buffers_count_mask;
        }

        // Wait for all the queued requests to be cancelled
        pthis->mDataPort->AbortSending();
        for (int j = 0; j < buffers_count; j++)
        {
            long bytesToSend = buffer_size;
            pthis->mDataPort->WaitForSending(handles[j], 1000);
            pthis->mDataPort->FinishDataSending(&buffers[j*buffer_size], bytesToSend, handles[j]);
        }
#ifndef NDEBUG
        printf("Cyclic transmitting FULLY STOPPED\n");
#endif
        delete[] buffers;
        return 0;
    }, this);
    mTxCyclicRunning.store(true);

    return LMS_StreamBoard::SUCCESS;
}

/** @brief Stops cyclic transmitting thread
*/
LMS_StreamBoard::Status LMS_StreamBoard::StopCyclicTransmitting()
{
    stopTxCyclic.store(true);
    if (mTxCyclicRunning.load() == true)
    {
        threadTxCyclic.join();
        mTxCyclicRunning.store(false);
    }
    return LMS_StreamBoard::SUCCESS;
}
