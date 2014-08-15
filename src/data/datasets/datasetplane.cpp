/*
 * datasetplane.cpp
 *
 *  Created on: Apr 10, 2014
 *      Author: schurade
 */

#include "datasetplane.h"

#include "../models.h"

#include "../../gui/gl/glfunctions.h"

#include <QtOpenGL/QGLShaderProgram>

DatasetPlane::DatasetPlane() :
    Dataset( QDir( "new plane" ), Fn::DatasetType::PLANE ),
    vbo0( 0 ),
    m_handle0( GLFunctions::getPickIndex() ),
    m_handle1( GLFunctions::getPickIndex() ),
    m_handle2( GLFunctions::getPickIndex() ),
    dirty( true )
{
    QList<Dataset*>dsl = Models::getDatasets( Fn::DatasetType::NIFTI_ANY );

    int nx = 160;
    int ny = 200;
    int nz = 160;
    float dx = 1.0f;
    float dy = 1.0f;
    float dz = 1.0f;
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 0.0f;
    if ( dsl.size() > 0 )
    {
        nx = dsl[0]->properties().get( Fn::Property::D_NX ).toInt();
        ny = dsl[0]->properties().get( Fn::Property::D_NY ).toInt();
        nz = dsl[0]->properties().get( Fn::Property::D_NZ ).toInt();
        dx = dsl[0]->properties().get( Fn::Property::D_DX ).toFloat();
        dy = dsl[0]->properties().get( Fn::Property::D_DY ).toFloat();
        dz = dsl[0]->properties().get( Fn::Property::D_DZ ).toFloat();
        ax = dsl[0]->properties().get( Fn::Property::D_ADJUST_X ).toFloat();
        ay = dsl[0]->properties().get( Fn::Property::D_ADJUST_Y ).toFloat();
        az = dsl[0]->properties().get( Fn::Property::D_ADJUST_Z ).toFloat();
    }

    QVector3D h0( nx * dx / 2 + ax, ny * dy / 2 + ay, nz * dz / 2 + az );
    QVector3D h1( ax, h0.y(), h0.z() );
    QVector3D h2( h0.x(), ay, h0.z() );

    m_properties["maingl"].createBool( Fn::Property::D_SHOW_PLANE_HANDLES, true, "general" );
    m_properties["maingl"].createColor( Fn::Property::D_HANDLE_COLOR, QColor( 255, 0, 0 ), "general" );

    m_properties["maingl"].createVector( Fn::Property::D_HANDLE_0, h0, "handles" );
    m_properties["maingl"].createVector( Fn::Property::D_HANDLE_1, h1, "handles" );
    m_properties["maingl"].createVector( Fn::Property::D_HANDLE_2, h2, "handles" );

    PropertyGroup props2( m_properties["maingl"] );
    m_properties.insert( "maingl2", props2 );
    m_properties["maingl2"].createBool( Fn::Property::D_SHOW_PLANE_HANDLES, true, "general" );
    m_properties["maingl2"].createColor( Fn::Property::D_HANDLE_COLOR, QColor( 255, 0, 0 ), "general" );
}

DatasetPlane::~DatasetPlane()
{
}

void DatasetPlane::draw( QMatrix4x4 pMatrix, QMatrix4x4 mvMatrix, int width, int height, int renderMode, QString target )
{
    if ( !properties( target ).get( Fn::Property::D_ACTIVE ).toBool() )
    {
        return;
    }

    QVector3D h0 = m_properties["maingl"].get( Fn::Property::D_HANDLE_0 ).value<QVector3D>();
    QVector3D h1 = m_properties["maingl"].get( Fn::Property::D_HANDLE_1 ).value<QVector3D>();
    QVector3D h2 = m_properties["maingl"].get( Fn::Property::D_HANDLE_2 ).value<QVector3D>();

    if ( m_properties["maingl"].get( Fn::Property::D_SHOW_PLANE_HANDLES ).toBool() )
    {
        QColor color = m_properties["maingl"].get( Fn::Property::D_HANDLE_COLOR ).value<QColor>();
        color.setAlpha( 254 );
        GLFunctions::renderSphere( pMatrix, mvMatrix, h0.x(), h0.y(), h0.z(), 5, 5, 5,
                                  color, m_handle0, width, height, renderMode );
        GLFunctions::renderSphere( pMatrix, mvMatrix, h1.x(), h1.y(), h1.z(), 5, 5, 5,
                                  color, m_handle1, width, height, renderMode );
        GLFunctions::renderSphere( pMatrix, mvMatrix, h2.x(), h2.y(), h2.z(), 5, 5, 5,
                                  color, m_handle2, width, height, renderMode );
    }

    if ( dirty )
    {
        initGeometry();
    }

    float alpha = GLFunctions::sliceAlpha[target];

    switch ( renderMode )
    {
        case 0:
            break;
        case 1:
        {
            if ( alpha < 1.0 ) // obviously not opaque
            {
                return;
            }
            break;
        }
        default:
        {
            if ( alpha == 1.0  ) // not transparent
            {
                return;
            }
            break;
        }
    }

    if ( !GLFunctions::setupTextures() )
    {
        return;
    }
    QGLShaderProgram* program = GLFunctions::getShader( "slice" );

    program->bind();
    GLFunctions::f->glBindBuffer( GL_ARRAY_BUFFER, vbo0 );

    intptr_t offset = 0;
    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation( "a_position" );
    program->enableAttributeArray( vertexLocation );
    GLFunctions::f->glVertexAttribPointer( vertexLocation, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (const void *) offset );

    // Set modelview-projection matrix
    program->setUniformValue( "mvp_matrix", pMatrix * mvMatrix );

    program->setUniformValue( "u_alpha", alpha );
    program->setUniformValue( "u_renderMode", renderMode );
    program->setUniformValue( "u_canvasSize", width, height );
    program->setUniformValue( "D0", 9 );
    program->setUniformValue( "D1", 10 );
    program->setUniformValue( "D2", 11 );
    program->setUniformValue( "P0", 12 );

    GLFunctions::setTextureUniforms( program, target );

    // Draw cube geometry using indices from VBO 0
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
    GLFunctions::f->glBindBuffer( GL_ARRAY_BUFFER, 0 );

}
void DatasetPlane::initGeometry()
{
    if( vbo0 )
    {
        GLFunctions::f->glDeleteBuffers( 1, &vbo0 );
    }
    GLFunctions::f->glGenBuffers( 1, &vbo0 );

    QVector3D h0 = m_properties["maingl"].get( Fn::Property::D_HANDLE_0 ).value<QVector3D>();
    QVector3D h1 = m_properties["maingl"].get( Fn::Property::D_HANDLE_1 ).value<QVector3D>();
    QVector3D h2 = m_properties["maingl"].get( Fn::Property::D_HANDLE_2 ).value<QVector3D>();

    float x0 = h1.x();
    float y0 = h2.y();
    float z0 = h0.z() - ( h0.z() - h1.z() ) - ( h0.z() - h2.z() );

    float x1 = h1.x();
    float y1 = h0.y() + h0.y() - h2.y();
    float z1 = h0.z() - ( h0.z() - h1.z() ) + ( h0.z() - h2.z() );

    float x2 = h0.x() + h0.x() - h1.x();
    float y2 = h0.y() + h0.y() - h2.y();
    float z2 = h0.z() + ( h0.z() - h1.z() ) + ( h0.z() - h2.z() );

    float x3 = h0.x() + h0.x() - h1.x();
    float y3 = h2.y();
    float z3 = h0.z() + ( h0.z() - h1.z() ) - ( h0.z() - h2.z() );

    float vertices[] =
    {
        x0, y0, z0,
        x1, y1, z1,
        x3, y3, z3,
        x2, y2, z2,
    };

    // Transfer vertex data to VBO 1
    GLFunctions::f->glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbo0 );
    GLFunctions::f->glBufferData( GL_ELEMENT_ARRAY_BUFFER, 4 * 3 * sizeof(GLfloat), vertices, GL_STATIC_DRAW );
    GLFunctions::f->glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    dirty = false;
}

bool DatasetPlane::rightMouseDrag( int pickId, QVector3D dir, Qt::KeyboardModifiers modifiers, QString target )
{
    QVector3D h0 = m_properties["maingl"].get( Fn::Property::D_HANDLE_0 ).value<QVector3D>();
    QVector3D h1 = m_properties["maingl"].get( Fn::Property::D_HANDLE_1 ).value<QVector3D>();
    QVector3D h2 = m_properties["maingl"].get( Fn::Property::D_HANDLE_2 ).value<QVector3D>();

    if ( pickId == m_handle0 )
    {
        h0.setX( h0.x() + dir.x() );
        h0.setY( h0.y() + dir.y() );
        h0.setZ( h0.z() + dir.z() );
        h1.setX( h1.x() + dir.x() );
        h1.setY( h1.y() + dir.y() );
        h1.setZ( h1.z() + dir.z() );
        h2.setX( h2.x() + dir.x() );
        h2.setY( h2.y() + dir.y() );
        h2.setZ( h2.z() + dir.z() );

        m_properties["maingl"].set( Fn::Property::D_HANDLE_0, h0 );
        m_properties["maingl"].set( Fn::Property::D_HANDLE_1, h1 );
        m_properties["maingl"].set( Fn::Property::D_HANDLE_2, h2 );

        dirty = true;
        return true;
    }
    else if ( pickId == m_handle1 )
    {
        h1.setX( h1.x() + dir.x() );
        h1.setY( h0.y() );
        h1.setZ( h1.z() + dir.z() );
        m_properties["maingl"].set( Fn::Property::D_HANDLE_1, h1 );
        dirty = true;
        return true;
    }
    else if ( pickId == m_handle2 )
    {
        h2.setX( h0.x() );
        h2.setY( h2.y() + dir.y() );
        h2.setZ( h2.z() + dir.z() );
        m_properties["maingl"].set( Fn::Property::D_HANDLE_2, h2 );
        dirty = true;
        return true;
    }

    return false;
}
