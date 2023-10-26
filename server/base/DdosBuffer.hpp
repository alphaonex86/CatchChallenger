#ifndef DDOSBUFFER_H
#define DDOSBUFFER_H

#include <vector>
#include <string>
#include <iostream>
#ifdef CATCHCHALLENGER_EXTRA_CHECK
#include <sys/time.h>
#endif

template <class T,uint8_t maxItems>
class DdosBuffer
{
public:
    DdosBuffer();
    T total() const;  // sum of all elements
    void incrementLastValue();// increment the last value
    void flush();            // flush the oldest tab value if the array is at the biggest size
    void reset();
    std::string dump();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    uint64_t get_lastFlushTime();
    #endif
private:
    uint8_t m_start,m_stop;
    T m_total,m_newValue;
    T m_elems[maxItems];     // elements
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    uint64_t lastFlush;
    #endif
};

template <class T,uint8_t maxItems>
DdosBuffer<T,maxItems>::DdosBuffer() :
    m_start(0),
    m_stop(0),
    m_total(0),
    m_newValue(0)
{
    if(maxItems<1)
    {
        std::cerr << "circularbuffer can't have 0 element" << std::endl;
        abort();
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    struct timeval tv;
    gettimeofday(&tv,NULL);
    lastFlush=(uint64_t)tv.tv_sec;
    #endif
}

template <class T,uint8_t maxItems>
T DdosBuffer<T,maxItems>::total() const
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    struct timeval tv;
    gettimeofday(&tv,NULL);
    if(lastFlush>(uint64_t)tv.tv_sec)
    {
        std::cerr << "time drift into DDOS, return 0" << std::endl;
        return 0;
    }
    if(((uint64_t)tv.tv_sec-lastFlush)>300)
    {
        std::cerr << "no flush into >300s, maybe to forgot call the DdosBuffer::flush() via timer, return 0" << std::endl;
        return 0;
    }
    #endif
    if /*constexpr */(std::is_same<T, uint8_t>::value)
    {
        const unsigned int &t=(unsigned int)m_total+(unsigned int)m_newValue;
        if(t<=255)
            return t;
        else
            return 255;
    }
    else if /*constexpr */(std::is_same<T, uint16_t>::value)
    {
        const unsigned int &t=(unsigned int)m_total+(unsigned int)m_newValue;
        if(t<=65535)
            return t;
        else
            return 65535;
    }
    else
        return m_total+m_newValue;
}

template <class T,uint8_t maxItems>
void DdosBuffer<T,maxItems>::incrementLastValue()
{
    if /*constexpr */(std::is_same<T, uint8_t>::value)
    {
        if(m_newValue<256)
            m_newValue++;
    }
    else if /*constexpr */(std::is_same<T, uint16_t>::value)
    {
        if(m_newValue<65536)
            m_newValue++;
    }
    else
        m_newValue++;
}

#ifdef CATCHCHALLENGER_EXTRA_CHECK
template <class T,uint8_t maxItems>
uint64_t DdosBuffer<T,maxItems>::get_lastFlushTime()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    if(lastFlush>(uint64_t)tv.tv_sec)
    {
        std::cerr << "time drift into DDOS, return 0" << std::endl;
        return 0;
    }
    return (uint64_t)tv.tv_sec-lastFlush;
}
#endif

template <class T,uint8_t maxItems>
void DdosBuffer<T,maxItems>::flush()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    struct timeval tv;
    gettimeofday(&tv,NULL);
    lastFlush=(uint64_t)tv.tv_sec;
    #endif
    if(m_stop>=(maxItems-1))
    {
        //add the new entry
        m_stop=0;
        m_elems[maxItems-1]=m_newValue;
        m_total+=m_newValue;
        m_newValue=0;

        if(m_start==0)
        {
            //at max, flush the oldest entrie
            m_start++;
            m_total-=m_elems[0];
        }
    }
    else
    {
        //add the new entry
        m_elems[m_stop]=m_newValue;
        m_total+=m_newValue;
        m_newValue=0;
        m_stop++;

        if(m_stop==m_start)
        {
            //at max, flush the oldest entrie
            if(m_start>=(maxItems-1))
            {
                m_total-=m_elems[m_start];
                m_start=0;
            }
            else
            {
                m_total-=m_elems[m_start];
                m_start++;
            }
        }
    }
}

template <class T,uint8_t maxItems>
void DdosBuffer<T,maxItems>::reset()
{
    m_start=0;
    m_stop=0;
    m_total=0;
    m_newValue=0;
}

template <class T,uint8_t maxItems>
std::string DdosBuffer<T,maxItems>::dump()
{
    std::string s;
    s+="m_start: "+std::to_string(m_start);
    s+=", m_stop: "+std::to_string(m_stop);
    s+=", m_total: "+std::to_string(m_total);
    s+=", m_newValue: "+std::to_string(m_newValue);
    s+=", m_stop: "+std::to_string(m_stop);
    s+=", m_elems: [";
    unsigned int index=0;
    while(index<maxItems)
    {
        if(index>0)
        s+=",";
        s+=std::to_string(m_elems[index]);
        index++;
    }
    s+="]";
    return s;
}

#endif // DDOSBUFFER_H
