/*
 * singleshrenderer.cpp
 *
 * Created on: 12.07.2012
 * @author Ralph Schurade
 */
#include "singleshrenderer.h"

#include "glfunctions.h"
#include "arcball.h"

#include "../../data/datasets/datasetdwi.h"
#include "../../data/enums.h"
#include "../../data/models.h"
#include "../../data/vptr.h"
#include "../../algos/fmath.h"
#include "../../algos/qball.h"

#include "../../data/mesh/tesselation.h"

#include "../../thirdparty/newmat10/newmat.h"

#include <QGLShaderProgram>
#include <QDebug>
#include <QVector3D>
#include <QMatrix4x4>

#include <limits>

SingleSHRenderer::SingleSHRenderer() :
    vboIds( new GLuint[ 2 ] ),
    m_width( 1 ),
    m_height( 1 ),
    m_ratio( 1.0 ),
    m_dx( 1.0 ),
    m_dy( 1.0 ),
    m_dz( 1.0 ),
    m_previousSettings( "" )
{
    m_arcBall = new ArcBall( 50, 50 );
}

SingleSHRenderer::~SingleSHRenderer()
{
}

void SingleSHRenderer::init()
{
}

void SingleSHRenderer::initGL()
{
    initializeOpenGLFunctions();
    glGenBuffers( 2, vboIds );

    glClearColor( 0.0, 0.0, 0.0, 1.0 );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glEnable( GL_DEPTH_TEST );

    glEnable( GL_MULTISAMPLE );
}

void SingleSHRenderer::resizeGL( int width, int height )
{
    m_width = width;
    m_height = height;

    m_arcBall->set_win_size( width, height );
    glViewport( 0, 0, m_width, m_height );
    calcMVPMatrix();
}

void SingleSHRenderer::leftMouseDown( int x, int y )
{
    m_arcBall->click( x, y );
    calcMVPMatrix();
}

void SingleSHRenderer::leftMouseDrag( int x, int y )
{
    m_arcBall->drag( x, y );
    calcMVPMatrix();
}

void SingleSHRenderer::calcMVPMatrix()
{
    m_ratio = static_cast< float >( m_width ) / static_cast< float >( m_height );

    m_arcBall->setRotCenter( 1., 1., 1. );

    // Reset projection
    QMatrix4x4 pMatrix;
    pMatrix.setToIdentity();


    if ( m_ratio >= 1.0 )
    {
        pMatrix.ortho( -m_ratio, m_ratio, -1., 1., -3000, 3000 );
    }
    else
    {
        pMatrix.ortho( -1., 1., -1. / m_ratio, 1. / m_ratio, -3000, 3000 );

    }

    m_mvpMatrix = pMatrix * m_arcBall->getMVMat();
}


void SingleSHRenderer::initGeometry()
{
    PropertyGroup props = m_dataset->properties();
    m_dx = props.get( Fn::Property::D_DX ).toFloat();
    m_dy = props.get( Fn::Property::D_DY ).toFloat();
    m_dz = props.get( Fn::Property::D_DZ ).toFloat();

    m_nx = props.get( Fn::Property::D_NX ).toInt();
    m_ny = props.get( Fn::Property::D_NY ).toInt();
    m_nz = props.get( Fn::Property::D_NZ ).toInt();

    int blockSize = m_nx * m_ny * m_nz;
    int dim = props.get( Fn::Property::D_DIM ).toInt();

    int xi = Models::g()->data( Models::g()->index( (int)Fn::Property::G_SAGITTAL, 0 ) ).toFloat();
    int yi = Models::g()->data( Models::g()->index( (int)Fn::Property::G_CORONAL, 0 ) ).toFloat();
    int zi = Models::g()->data( Models::g()->index( (int)Fn::Property::G_AXIAL, 0 ) ).toFloat();

    xi = qMax( 0, qMin( xi, m_nx - 1) );
    yi = qMax( 0, qMin( yi, m_ny - 1) );
    zi = qMax( 0, qMin( zi, m_nz - 1) );



    int lod = 4; //m_dataset->getProperty( "lod" ).toInt();

    const Matrix* vertices = tess::vertices( lod );
    const int* faces = tess::faces( lod );
    int numVerts = tess::n_vertices( lod );
    int numTris = tess::n_faces( lod );

    int order = props.get( Fn::Property::D_ORDER ).toInt();

    Matrix base = ( FMath::sh_base( (*vertices), order ) );

    QString s = createSettingsString( { xi, yi, zi } );

    if ( s == m_previousSettings )
    {
        return;
    }
    m_previousSettings = s;

    std::vector<float>* data = m_dataset->getData();

    std::vector<float>verts;
    verts.reserve( numVerts * 10 );
    std::vector<int>indexes;
    indexes.reserve( numTris * 3 );
    m_tris = 0;

    if ( ( fabs( data->at( xi + yi * m_nx + zi * m_nx * m_ny ) ) > 0.0001 ) )
    {
        ColumnVector dv = FMath::createVector( xi + yi * m_nx + zi * m_nx * m_ny, *data, blockSize, dim );
        ColumnVector r = base * dv;

        float max = 0;
        float min = std::numeric_limits<float>::max();
        for ( int i = 0; i < r.Nrows(); ++i )
        {
            max = qMax( max, (float)r(i+1) );
            min = qMin( min, (float)r(i+1) );
        }

        for ( int i = 0; i < r.Nrows(); ++i )
        {
            r(i+1) = r(i+1) / max * 0.8;
        }

        for ( int i = 0; i < numVerts; ++i )
        {
            verts.push_back( (*vertices)( i+1, 1 ) );
            verts.push_back( (*vertices)( i+1, 2 ) );
            verts.push_back( (*vertices)( i+1, 3 ) );
            verts.push_back( 1.0 );
            verts.push_back( 1.0 );
            verts.push_back( 1.0 );
            verts.push_back( r(i + 1) );
        }
        for ( int i = 0; i < numTris; ++i )
        {
            indexes.push_back( faces[i*3] );
            indexes.push_back( faces[i*3+1] );
            indexes.push_back( faces[i*3+2] );
        }
        m_tris = numTris * 3;
    }


    qDebug() << m_tris << " " << verts.size() << " " << indexes.size();

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vboIds[ 0 ] );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexes.size() * sizeof(GLuint), &indexes[0], GL_STATIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    glBindBuffer( GL_ARRAY_BUFFER, vboIds[ 1 ] );
    glBufferData( GL_ARRAY_BUFFER, verts.size() * sizeof(GLfloat), &verts[0], GL_STATIC_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

void SingleSHRenderer::setupTextures()
{
}

void SingleSHRenderer::setShaderVars()
{
    QGLShaderProgram* program = GLFunctions::getShader( "qball" );

    program->bind();

    intptr_t offset = 0;
    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation( "a_position" );
    program->enableAttributeArray( vertexLocation );
    glVertexAttribPointer( vertexLocation, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (const void *) offset );


    offset += sizeof(float) * 3;
    int offsetLocation = program->attributeLocation( "a_offset" );
    program->enableAttributeArray( offsetLocation );
    glVertexAttribPointer( offsetLocation, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (const void *) offset );

    offset += sizeof(float) * 3;
    int radiusLocation = program->attributeLocation( "a_radius" );
    program->enableAttributeArray( radiusLocation );
    glVertexAttribPointer( radiusLocation, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (const void *) offset );
}

void SingleSHRenderer::draw()
{
    QList<int>rl;

    int countDatasets = Models::d()->rowCount();
    for ( int i = 0; i < countDatasets; ++i )
    {
        QModelIndex index = Models::d()->index( i, (int)Fn::Property::D_ACTIVE );
        if ( Models::d()->data( index, Qt::DisplayRole ).toBool() )
        {
            index = Models::d()->index( i, (int)Fn::Property::D_TYPE );
            if ( Models::d()->data( index, Qt::DisplayRole ).toInt() == (int)Fn::DatasetType::NIFTI_SH )
            {
                rl.push_back( i );
            }
        }
    }

    if ( rl.size() > 0 )
    {
        m_dataset = VPtr<DatasetDWI>::asPtr( Models::d()->data( Models::d()->index( rl[0], (int)Fn::Property::D_DATASET_POINTER ), Qt::DisplayRole ) );
        initGeometry();
    }
    else
    {
        m_tris = 0;
    }

    if ( m_tris != 0 )
    {
        GLFunctions::getShader( "qball" )->bind();
        // Set modelview-projection matrix
        GLFunctions::getShader( "qball" )->setUniformValue( "mvp_matrix", m_mvpMatrix );
        GLFunctions::getShader( "qball" )->setUniformValue( "mv_matrixInvert", m_arcBall->getMVMat().inverted() );

        bool hnl = m_dataset->properties().get( Fn::Property::D_HIDE_NEGATIVE_LOBES ).toBool();
        GLFunctions::getShader( "qball" )->setUniformValue( "u_hideNegativeLobes", hnl );
        GLFunctions::getShader( "qball" )->setUniformValue( "u_renderMode", 0 );

        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vboIds[ 0 ] );
        glBindBuffer( GL_ARRAY_BUFFER, vboIds[ 1 ] );

        setShaderVars();

        glDrawElements( GL_TRIANGLES, m_tris, GL_UNSIGNED_INT, 0 );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
    }
}

QString SingleSHRenderer::createSettingsString( std::initializer_list<QVariant>settings )
{
    QString result("");

    for ( auto i = settings.begin(); i != settings.end(); ++i )
    {
        result += (*i).toString();
    }
    return result;
}
