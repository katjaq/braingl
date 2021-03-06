/*
 * fiberrendererthread.cpp
 *
 * Created on: 28.12.2012
 * @author Ralph Schurade
 */
#include "fiberrendererthread.h"

#include "glfunctions.h"

#include <QVector3D>

#include <math.h>

FiberRendererThread::FiberRendererThread( std::vector< std::vector<float> >* data, int id ) :
    m_data( data ),
    m_id( id )
{
    m_verts = new std::vector<float>();
    m_globalColors = new std::vector<QVector3D>;
}

FiberRendererThread::~FiberRendererThread()
{
    m_verts->clear();
    m_globalColors->clear();
}

std::vector<float>* FiberRendererThread::getVerts()
{
    return m_verts;
}

std::vector<QVector3D>* FiberRendererThread::getGlobalColors()
{
    return m_globalColors;
}

void FiberRendererThread::run()
{
    int numThreads = GLFunctions::idealThreadCount;

    int chunkSize = m_data->size() / numThreads;

    int begin = m_id * chunkSize;
    int end = m_id * chunkSize + chunkSize;

    try
    {
        m_verts->reserve( chunkSize * 6 );
    }
    catch ( std::bad_alloc& )
    {
        qCritical() << "***error*** failed to allocate enough memory for vbo";
        exit ( 0 );
    }

    if ( m_id == numThreads - 1 )
    {
        end = m_data->size();
    }

    // for all voxels:
    for ( int i = begin; i < end; ++i )
    {
        std::vector<float> fib = m_data->at(i);

        if ( fib.size() < 6 )
        {
            printf( "fib with size < 2 detected" );
            continue;
        }

        int numFloats = fib.size();
        QVector3D lineStart( fib[0], fib[1], fib[2] );
        QVector3D lineEnd( fib[numFloats-3], fib[numFloats-2], fib[numFloats-1] );

        QVector3D gc( fabs( lineStart.x() - lineEnd.x() ), fabs( lineStart.y() - lineEnd.y() ), fabs( lineStart.z() - lineEnd.z() ) );
        gc.normalize();
        m_globalColors->push_back( gc );

        // push back the first vertex, done seperately because of nomal calculation
        m_verts->push_back( fib[0] );
        m_verts->push_back( fib[1] );
        m_verts->push_back( fib[2] );

        QVector3D localColor( fabs( fib[0] - fib[3] ), fabs( fib[1] - fib[4] ), fabs( fib[2] - fib[5] ) );
        localColor.normalize();

        m_verts->push_back( localColor.x() );
        m_verts->push_back( localColor.y() );
        m_verts->push_back( localColor.z() );

        for ( unsigned int k = 1; k < fib.size() / 3 - 1; ++k )
        {
            m_verts->push_back( fib[k*3] );
            m_verts->push_back( fib[k*3+1] );
            m_verts->push_back( fib[k*3+2] );

            QVector3D localColor( fabs( fib[k*3-3] - fib[k*3+3] ), fabs( fib[k*3-2] - fib[k*3+4] ), fabs( fib[k*3-1] - fib[k*3+5] ) );
            localColor.normalize();

            m_verts->push_back( localColor.x() );
            m_verts->push_back( localColor.y() );
            m_verts->push_back( localColor.z() );

        }

        // push back the last vertex, done seperately because of nomal calculation
        m_verts->push_back( fib[numFloats-3] );
        m_verts->push_back( fib[numFloats-2] );
        m_verts->push_back( fib[numFloats-1] );

        QVector3D localColor2( fabs( fib[numFloats-6] - fib[numFloats-3] ), fabs( fib[numFloats-5] - fib[numFloats-2] ), fabs( fib[numFloats-4] - fib[numFloats-1] ) );
        localColor.normalize();

        m_verts->push_back( localColor2.x() );
        m_verts->push_back( localColor2.y() );
        m_verts->push_back( localColor2.z() );
    }
}
