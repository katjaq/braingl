/*
 * memeshRenderer.cpp
 *
 * Created on: Dec 4, 2012
 * @author Ralph Schurade
 */
#include "meshrenderer.h"

#include "glfunctions.h"

#include "../../data/enums.h"
#include "../../data/models.h"
#include "../../data/mesh/trianglemesh2.h"
#include "../../data/properties/propertygroup.h"

#include <QtOpenGL/QGLShaderProgram>
#include <QDebug>
#include <QVector3D>
#include <QMatrix4x4>

#include <limits>

MeshRenderer::MeshRenderer( TriangleMesh2* mesh ) :
    m_colorMode( 0 ),
    m_tris( 0 ),
    vboIds( new GLuint[ 3 ] ),
    m_pickId( GLFunctions::getPickIndex() ),
    m_mesh( mesh ),
    m_dirty( true ),
    m_renderMode( 0 ),
    m_colormap( 1 ),
    m_selectedMin( 0.0 ),
    m_selectedMax( 1.0 ),
    m_lowerThreshold( 0.0 ),
    m_upperThreshold( 1.0 )
{

}

MeshRenderer::~MeshRenderer()
{
    glDeleteBuffers(1, &( vboIds[ 0 ] ) );
    glDeleteBuffers(1, &( vboIds[ 1 ] ) );
    glDeleteBuffers(1, &( vboIds[ 2 ] ) );
}

void MeshRenderer::setMesh( TriangleMesh2* mesh )
{
    m_mesh = mesh;
    m_dirty = true;
}

void MeshRenderer::init()
{
    initializeOpenGLFunctions();
    glGenBuffers( 3, vboIds );
}

void MeshRenderer::draw( QMatrix4x4 p_matrix, QMatrix4x4 mv_matrix, int width, int height, int renderMode, PropertyGroup& props )
{
    if ( m_mesh->numTris() == 0 )
    {
        return;
    }

    float alpha = props.get( Fn::Property::D_ALPHA ).toFloat();
    m_renderMode = renderMode;

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

    setRenderParams( props );

    QGLShaderProgram* program;

    if ( props.get( Fn::Property::D_INTERPOLATION ).toBool() )
    {
        program = GLFunctions::getShader( "mesh" );
    }
    else
    {
        program = GLFunctions::getShader( "mesh2" );
    }

    program->bind();

    GLFunctions::setupTextures();
    GLFunctions::setTextureUniforms( program, "maingl" );
    // Set modelview-projection matrix
    program->setUniformValue( "mvp_matrix", p_matrix * mv_matrix * m_mMatrix );
    program->setUniformValue( "mv_matrixInvert", ( mv_matrix * m_mMatrix ).inverted() );
    program->setUniformValue( "userTransformMatrix", props.get( Fn::Property::D_TRANSFORM ).value<QMatrix4x4>() );

    program->setUniformValue( "u_colorMode", m_colorMode );
    program->setUniformValue( "u_colormap", m_colormap );
    program->setUniformValue( "u_color", m_color.redF(), m_color.greenF(), m_color.blueF(), 1.0 );
    program->setUniformValue( "u_selectedMin", m_selectedMin );
    program->setUniformValue( "u_selectedMax", m_selectedMax );
    program->setUniformValue( "u_lowerThreshold", m_lowerThreshold );
    program->setUniformValue( "u_upperThreshold", m_upperThreshold );

    float sx = Models::getGlobal( Fn::Property::G_SAGITTAL ).toFloat();
    float sy = Models::getGlobal( Fn::Property::G_CORONAL ).toFloat();
    float sz = Models::getGlobal( Fn::Property::G_AXIAL ).toFloat();

    program->setUniformValue( "u_x", sx ); // + dx / 2.0f );
    program->setUniformValue( "u_y", sy ); // + dy / 2.0f );
    program->setUniformValue( "u_z", sz ); // + dz / 2.0f );
    program->setUniformValue( "u_cutLowerX", props.get( Fn::Property::D_MESH_CUT_LOWER_X ).toBool() );
    program->setUniformValue( "u_cutLowerY", props.get( Fn::Property::D_MESH_CUT_LOWER_Y ).toBool() );
    program->setUniformValue( "u_cutLowerZ", props.get( Fn::Property::D_MESH_CUT_LOWER_Z ).toBool() );
    program->setUniformValue( "u_cutHigherX", props.get( Fn::Property::D_MESH_CUT_HIGHER_X ).toBool() );
    program->setUniformValue( "u_cutHigherY", props.get( Fn::Property::D_MESH_CUT_HIGHER_Y ).toBool() );
    program->setUniformValue( "u_cutHigherZ", props.get( Fn::Property::D_MESH_CUT_HIGHER_Z ).toBool() );

    program->setUniformValue( "u_adjustX", props.get( Fn::Property::D_ADJUST_X ).toFloat() );
    program->setUniformValue( "u_adjustY", props.get( Fn::Property::D_ADJUST_Y ).toFloat() );
    program->setUniformValue( "u_adjustZ", props.get( Fn::Property::D_ADJUST_Z ).toFloat() );

    program->setUniformValue( "u_alpha", alpha );
    program->setUniformValue( "u_renderMode", renderMode );
    program->setUniformValue( "u_canvasSize", width, height );
    program->setUniformValue( "D0", 9 );
    program->setUniformValue( "D1", 10 );
    program->setUniformValue( "D2", 11 );
    program->setUniformValue( "P0", 12 );

    program->setUniformValue( "u_lighting", props.get( Fn::Property::D_LIGHT_SWITCH ).toBool() );
    program->setUniformValue( "u_lightAmbient", props.get( Fn::Property::D_LIGHT_AMBIENT ).toFloat() );
    program->setUniformValue( "u_lightDiffuse", props.get( Fn::Property::D_LIGHT_DIFFUSE ).toFloat() );
    program->setUniformValue( "u_materialAmbient", props.get( Fn::Property::D_MATERIAL_AMBIENT ).toFloat() );
    program->setUniformValue( "u_materialDiffuse", props.get( Fn::Property::D_MATERIAL_DIFFUSE ).toFloat() );
    program->setUniformValue( "u_materialSpecular", props.get( Fn::Property::D_MATERIAL_SPECULAR ).toFloat() );
    program->setUniformValue( "u_materialShininess", props.get( Fn::Property::D_MATERIAL_SHININESS ).toFloat() );
    program->setUniformValue( "u_meshTransparency", Models::getGlobal( Fn::Property::G_MESH_TRANSPARENCY ).toInt() );


    float pAlpha =  1.0;
    float blue = (float) ( ( m_pickId ) & 0xFF ) / 255.f;
    float green = (float) ( ( m_pickId >> 8 ) & 0xFF ) / 255.f;
    float red = (float) ( ( m_pickId >> 16 ) & 0xFF ) / 255.f;
    program->setUniformValue( "u_pickColor", red, green , blue, pAlpha );

    if ( m_dirty )
    {
        initGeometry();
    }

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vboIds[ 0 ] );
    glBindBuffer( GL_ARRAY_BUFFER, vboIds[ 1 ] );
    setShaderVars( program );

    glEnable(GL_CULL_FACE);
    glCullFace( GL_BACK );

    if ( props.get( Fn::Property::D_INVERT_VERTEX_ORDER ).toBool() )
    {
        glFrontFace( GL_CCW );
    }
    else
    {
        glFrontFace( GL_CW );
    }

    if ( props.get( Fn::Property::D_RENDER_WIREFRAME ).toBool() )
    {
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    }

    int firstI = props.get( Fn::Property::D_START_INDEX ).toInt();
    int lastI = props.get( Fn::Property::D_END_INDEX ).toInt();
    if ( firstI < lastI )
    {
        glDrawElements( GL_TRIANGLES, (lastI-firstI)*3, GL_UNSIGNED_INT, (void*)(firstI*3*sizeof(GLuint)) );
    }

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

    glDisable(GL_CULL_FACE);

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

void MeshRenderer::setShaderVars( QGLShaderProgram* program )
{
    intptr_t offset = 0;

    int bufferSize = m_mesh->bufferSize();

    int vertexLocation = program->attributeLocation( "a_position" );
    program->enableAttributeArray( vertexLocation );
    glVertexAttribPointer( vertexLocation, 3, GL_FLOAT, GL_FALSE, sizeof(float) * bufferSize, (const void *) offset );
    offset += sizeof(float) * 3;

    int normalLocation = program->attributeLocation( "a_normal" );
    program->enableAttributeArray( normalLocation );
    glVertexAttribPointer( normalLocation, 3, GL_FLOAT, GL_FALSE, sizeof(float) * bufferSize, (const void *) offset );
    offset += sizeof(float) * 3;

    int valueLocation = program->attributeLocation( "a_value" );
    program->enableAttributeArray( valueLocation );
    glVertexAttribPointer( valueLocation, 1, GL_FLOAT, GL_FALSE, sizeof(float) * bufferSize, (const void *) offset );
    offset += sizeof(float) * 1;

    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    glBindBuffer( GL_ARRAY_BUFFER, vboIds[ 2 ] );
    int colorLocation = program->attributeLocation( "a_color" );
    program->enableAttributeArray( colorLocation );
    glVertexAttribPointer( colorLocation, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0 );
}

void MeshRenderer::initGeometry()
{
    int bufferSize = m_mesh->bufferSize();

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vboIds[ 0 ] );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_mesh->numTris() * 3 * sizeof(GLuint), m_mesh->getIndexes(), GL_STATIC_DRAW );

    glBindBuffer( GL_ARRAY_BUFFER, vboIds[ 1 ] );
    glBufferData( GL_ARRAY_BUFFER, m_mesh->numVerts() * bufferSize * sizeof(GLfloat), m_mesh->getVertices(), GL_STATIC_DRAW );

    glBindBuffer( GL_ARRAY_BUFFER, vboIds[ 2 ] );
    glBufferData( GL_ARRAY_BUFFER, m_mesh->numVerts() * 4 * sizeof(GLfloat), m_mesh->getVertexColors(), GL_DYNAMIC_DRAW );

    m_dirty = false;
}

void MeshRenderer::setRenderParams( PropertyGroup& props )
{
    m_colorMode = props.get( Fn::Property::D_COLORMODE ).toInt();
    m_colormap = props.get( Fn::Property::D_COLORMAP ).toInt();
    m_selectedMin = props.get( Fn::Property::D_SELECTED_MIN ).toFloat();
    m_selectedMax = props.get( Fn::Property::D_SELECTED_MAX ).toFloat();
    m_lowerThreshold = props.get( Fn::Property::D_LOWER_THRESHOLD ).toFloat();
    m_upperThreshold = props.get( Fn::Property::D_UPPER_THRESHOLD ).toFloat();
    m_color = props.get( Fn::Property::D_COLOR ).value<QColor>();

    m_mMatrix.setToIdentity();

    if( props.contains( Fn::Property::D_ROTATE_X ) )
    {
        m_mMatrix.rotate( props.get( Fn::Property::D_ROTATE_X ).toFloat(), 1.0, 0.0, 0.0 );
        m_mMatrix.rotate( props.get( Fn::Property::D_ROTATE_Y ).toFloat(), 0.0, 1.0, 0.0 );
        m_mMatrix.rotate( props.get( Fn::Property::D_ROTATE_Z ).toFloat(), 0.0, 0.0, 1.0 );
        m_mMatrix.scale( props.get( Fn::Property::D_SCALE_X ).toFloat(),
                          props.get( Fn::Property::D_SCALE_Y ).toFloat(),
                          props.get( Fn::Property::D_SCALE_Z ).toFloat() );
    }
}

void MeshRenderer::beginUpdateColor()
{
    glBindBuffer( GL_ARRAY_BUFFER, vboIds[ 2 ] );
    m_colorBufferPointer = (float*)glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY );
}

void MeshRenderer::endUpdateColor()
{
    glUnmapBuffer( GL_ARRAY_BUFFER );
    m_colorBufferPointer = 0;
}

void MeshRenderer::updateColor( int id, float r, float g, float b, float a )
{
    if ( id != -1 )
    {
        if( m_colorBufferPointer )
        {
            m_colorBufferPointer[ id * 4 ] = r;
            m_colorBufferPointer[ id * 4 + 1] = g;
            m_colorBufferPointer[ id * 4 + 2] = b;
            m_colorBufferPointer[ id * 4 + 3] = a;
        }
    }
}
