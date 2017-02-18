#ifndef DDOSBUFFER_H
#define DDOSBUFFER_H

#include <vector>
#include <iostream>

template <class T,uint8_t maxItems>
class DdosBuffer
{
public:
    DdosBuffer();
    T total() const;  // sum of all elements
    void incrementLastValue();// increment the last value
    void flush();            // flush the oldest tab value if the array is at the biggest size
    void reset();
private:
    uint8_t m_start,m_stop;
    T m_total,m_newValue;
    T m_elems[maxItems];     // elements
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
}

template <class T,uint8_t maxItems>
T DdosBuffer<T,maxItems>::total() const
{
    return m_total+m_newValue;
}

template <class T,uint8_t maxItems>
void DdosBuffer<T,maxItems>::incrementLastValue()
{
    m_newValue++;
}

template <class T,uint8_t maxItems>
void DdosBuffer<T,maxItems>::flush()
{
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

#endif // DDOSBUFFER_H
