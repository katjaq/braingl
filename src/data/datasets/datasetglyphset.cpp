/*
 * datasetglyphset.cpp
 *
 *  Created on: Apr 24, 2013
 *      Author: boettgerj
 */

#include "datasetglyphset.h"
#include "datasetcons.h"

#include "../properties/propertyfloat.h"

#include "../mesh/trianglemesh2.h"

#include "../../gui/gl/glfunctions.h"
#include "../../gui/gl/colormapfunctions.h"

#include "../../algos/colormapbase.h"
#include "../../algos/connection.h"
#include "../../algos/connections.h"

#include "../models.h"

#include "qfile.h"
#include "qfiledialog.h"

#include "qmath.h"

#include <qcoreapplication.h>

DatasetGlyphset::DatasetGlyphset( QDir filename, float minThreshold, float maxThreshold ) :
                DatasetCorrelation( filename, minThreshold, maxThreshold, Fn::DatasetType::GLYPHSET ),
                m_is_split( false ),
                consArray( NULL ),
                diffsArray( NULL ),
                vecsArray( NULL ),
                pieArrays( NULL ),
                numbers( NULL ),
                m_prenderer( NULL ),
                m_dprenderer( NULL ),
                m_vrenderer( NULL ),
                m_pierenderer( NULL ),
                prevGeo( -1 ),
                prevGlyph( -1 ),
                prevCol( -1 ),
                prevGlyphstyle( -1 ),
                prevColorMode( -1 ),
                prevThreshSign( 0 ),
                prevThresh( -1 ),
                prevMinlength( -1 ),
                glyphsChanged( false ),
                m_colors_name( "" ),
                littleBrains( std::vector<LittleBrainRenderer*>() ),
                littleMeshes( std::vector<TriangleMesh2*>() ),
                shifts1( std::vector<QVector3D>() ),
                shifts2( std::vector<QVector3D>() )
{
    addProperties();

    finalizeProperties();

    m_properties["maingl"].createBool( Fn::Property::D_DRAW_GLYPHS, true, "general" );
    m_properties["maingl2"].createBool( Fn::Property::D_DRAW_GLYPHS, false );
    m_properties["maingl"].createBool( Fn::Property::D_LITTLE_BRAIN_VISIBILITY, true, "general" );
    m_properties["maingl"].createList( Fn::Property::D_LITTLE_BRAINS_COLORMODE,
    { "per mesh", "mri", "per vertex", "vertex data" }, 3, "general" );

    connect( m_properties["maingl"].getProperty( Fn::Property::D_GLYPH_COLORMODE ), SIGNAL( valueChanged(QVariant)), this,
            SLOT( colorModeChanged(QVariant) ) );
    connect( m_properties["maingl"].getProperty( Fn::Property::D_GLYPHSTYLE ), SIGNAL( valueChanged(QVariant)), this,
            SLOT( glyphStyleChanged(QVariant) ) );
    connect( m_properties["maingl"].getProperty( Fn::Property::D_GLYPH_ROTATION ), SIGNAL( valueChanged(QVariant)), this,
            SLOT( rotationChanged(QVariant) ) );
    connect( m_properties["maingl"].getProperty( Fn::Property::D_LITTLE_BRAIN_VISIBILITY ), SIGNAL( valueChanged(QVariant)), this,
            SLOT( littleBrainVisibilityChanged(QVariant) ) );

    m_properties["maingl"].getWidget( Fn::Property::D_GLYPH_ROT_X )->setHidden( true );
    m_properties["maingl"].getWidget( Fn::Property::D_GLYPH_ROT_Y )->setHidden( true );
    m_properties["maingl"].getWidget( Fn::Property::D_GLYPH_ROT_Z )->setHidden( true );
}

DatasetGlyphset::~DatasetGlyphset()
{
    if ( m_prenderer )
    {
        delete m_prenderer;
        m_prenderer = NULL;
    }
    if ( m_vrenderer )
    {
        delete m_vrenderer;
        m_vrenderer = NULL;
    }
    if ( m_pierenderer )
    {
        delete m_pierenderer;
        m_pierenderer = NULL;
    }
    if ( m_dprenderer )
    {
        delete m_dprenderer;
        m_dprenderer = NULL;
    }
    if ( m_correlations )
    {
        delete m_correlations;
        m_correlations = NULL;
    }
}

void DatasetGlyphset::addSecondSurfaceSelector()
{
    m_properties["maingl2"].createList( Fn::Property::D_SURFACE, m_displayList, 0, "general" );
}

void DatasetGlyphset::addProperties()
{
    m_properties["maingl"].createFloat( Fn::Property::D_THRESHOLD, m_minThreshold, m_minThreshold, 1.0f, "general" );
    m_properties["maingl"].createFloat( Fn::Property::D_THRESHOLD_PERC, 0.5f, 0.0f, 1.0f, "general" );
    m_properties["maingl"].createList( Fn::Property::D_GLYPH_THRESHOLD_SIGN,
    { "+", "-", "+/-" }, 0, "general" );
    m_properties["maingl"].createList( Fn::Property::D_GLYPHSTYLE,
    { "points", "vectors", "pies", "diffpoints", "off" }, 4, "glyphs" ); //0 = points, 1 = vectors, 2 = pies
    m_properties["maingl"].createFloat( Fn::Property::D_GLYPHRADIUS, 0.01f, 0.0f, 0.5f, "glyphs" );
    m_properties["maingl"].createFloat( Fn::Property::D_NORMALIZATION, 0.5f, 0.0f, 1.0f, "glyphs" );
    m_properties["maingl"].getWidget( Fn::Property::D_NORMALIZATION )->setHidden( true );
    m_properties["maingl"].createFloat( Fn::Property::D_PRIMSIZE, 0.5f, 0.0f, 10.0f, "glyphs" );
    m_properties["maingl"].createFloat( Fn::Property::D_MINLENGTH, 0.0f, 0.0f, 100.0f, "general" );
    m_properties["maingl"].createBool( Fn::Property::D_DRAW_SURFACE, true, "general" );
    //m_properties["maingl"].create( Fn::Property::D_DRAW_GLYPHS, true, "general" );
    m_properties["maingl"].createBool( Fn::Property::D_GLYPH_ROTATION, false, "glyphs" );
    m_properties["maingl"].createFloat( Fn::Property::D_GLYPH_ROT_X, 0.0f, 0.0f, 360.0f, "glyphs" );
    m_properties["maingl"].createFloat( Fn::Property::D_GLYPH_ROT_Y, 0.0f, 0.0f, 360.0f, "glyphs" );
    m_properties["maingl"].createFloat( Fn::Property::D_GLYPH_ROT_Z, 0.0f, 0.0f, 360.0f, "glyphs" );
    m_properties["maingl"].createFloat( Fn::Property::D_GLYPH_ALPHA, 1.0f, 0.0f, 1.0f, "glyphs" );

    m_properties["maingl"].getProperty( Fn::Property::D_MIN )->setMin( -1 );
    m_properties["maingl"].getProperty( Fn::Property::D_MIN )->setValue( -1 );

    m_properties["maingl"].getProperty( Fn::Property::D_SELECTED_MIN )->setMin( -1 );
    m_properties["maingl"].getProperty( Fn::Property::D_SELECTED_MIN )->setValue( -1 );

    m_properties["maingl"].getProperty( Fn::Property::D_LOWER_THRESHOLD )->setMin( -1 );
    m_properties["maingl"].getProperty( Fn::Property::D_LOWER_THRESHOLD )->setValue( -1 );

    GLFunctions::createColormapBarProps( m_properties["maingl"] );

    ( (PropertyFloat*) m_properties["maingl"].getProperty( Fn::Property::D_GLYPHRADIUS ) )->setDigits( 4 );
    ( (PropertyFloat*) m_properties["maingl"].getProperty( Fn::Property::D_THRESHOLD_PERC ) )->setDigits( 4 );
    m_properties["maingl"].createList( Fn::Property::D_GLYPH_COLORMODE,
    { "orientation", "value" }, 0, "glyphs" );

    m_properties["maingl"].createButton( Fn::Property::D_COPY_COLORS, "general" );
    connect( m_properties["maingl"].getProperty( Fn::Property::D_COPY_COLORS ), SIGNAL( valueChanged( QVariant ) ), this, SLOT( slotCopyColors() ) );

    connect( m_properties["maingl"].getProperty( Fn::Property::D_THRESHOLD ), SIGNAL( valueChanged( QVariant ) ), this,
            SLOT( thresholdChanged(QVariant) ) );
    connect( m_properties["maingl"].getProperty( Fn::Property::D_THRESHOLD_PERC ), SIGNAL( valueChanged( QVariant ) ), this,
            SLOT( thresholdPercChanged(QVariant) ) );
}

void DatasetGlyphset::colorModeChanged( QVariant qv )
{
    if ( qv == 1 )
    {
        m_properties["maingl"].getWidget( Fn::Property::D_SURFACE_GLYPH_COLOR )->setDisabled( true );
    }
    else
    {
        m_properties["maingl"].getWidget( Fn::Property::D_SURFACE_GLYPH_COLOR )->setDisabled( false );
    }
}

void DatasetGlyphset::glyphStyleChanged( QVariant qv )
{
    if ( qv != 2 )
    {
        //Geometry-based glyphs
        m_properties["maingl"].getWidget( Fn::Property::D_NORMALIZATION )->setHidden( true );
        m_properties["maingl"].getWidget( Fn::Property::D_PRIMSIZE )->setHidden( false );
        m_properties["maingl"].getWidget( Fn::Property::D_SURFACE_GLYPH_GEOMETRY )->setHidden( false );
        m_properties["maingl"].getWidget( Fn::Property::D_GLYPH_COLORMODE )->setDisabled( false );
    }
    else
    {
        //Piechart glyphs
        m_properties["maingl"].getWidget( Fn::Property::D_NORMALIZATION )->setHidden( false );
        m_properties["maingl"].getWidget( Fn::Property::D_PRIMSIZE )->setHidden( true );
        m_properties["maingl"].getWidget( Fn::Property::D_SURFACE_GLYPH_GEOMETRY )->setHidden( true );
        m_properties["maingl2"].set( Fn::Property::D_GLYPH_COLORMODE, 0 );
        m_properties["maingl"].getWidget( Fn::Property::D_GLYPH_COLORMODE )->setDisabled( false );
    }
}

void DatasetGlyphset::rotationChanged( QVariant qv )
{
    if ( !qv.toBool() )
    {
        m_properties["maingl"].getWidget( Fn::Property::D_GLYPH_ROT_X )->setHidden( true );
        m_properties["maingl"].getWidget( Fn::Property::D_GLYPH_ROT_Y )->setHidden( true );
        m_properties["maingl"].getWidget( Fn::Property::D_GLYPH_ROT_Z )->setHidden( true );
    }
    else
    {
        m_properties["maingl"].getWidget( Fn::Property::D_GLYPH_ROT_X )->setHidden( false );
        m_properties["maingl"].getWidget( Fn::Property::D_GLYPH_ROT_Y )->setHidden( false );
        m_properties["maingl"].getWidget( Fn::Property::D_GLYPH_ROT_Z )->setHidden( false );
    }
}

void DatasetGlyphset::littleBrainVisibilityChanged( QVariant qv )
{
    if ( !qv.toBool() )
    {
        m_properties["maingl"].getWidget( Fn::Property::D_LITTLE_BRAINS_COLORMODE )->setHidden( true );
    }
    else
    {
        m_properties["maingl"].getWidget( Fn::Property::D_LITTLE_BRAINS_COLORMODE )->setHidden( false );
    }
}

void DatasetGlyphset::readConnectivity( QString filename )
{
    m_correlations = new CorrelationMatrix( filename );

    m_correlations->makeHistogram(roi);
    m_n = m_correlations->getN();
    //m_n = 0;
    qDebug() << "connectivity read";
    m_properties["maingl"].createInt( Fn::Property::D_GLYPHSET_PICKED_ID, -1, -1, m_n - 1, "general" );
}

void DatasetGlyphset::addCorrelation( float** corr )
{
    DatasetCorrelation::setCorrelationMatrix( corr );
    m_correlations->makeHistogram(roi);
    m_n = m_mesh[0]->numVerts();
    m_properties["maingl"].createInt( Fn::Property::D_GLYPHSET_PICKED_ID, -1, -1, m_n - 1, "general" );
}

void DatasetGlyphset::deleteRowFromMatrix()
{
    m_n = m_correlations->getN();
    for ( int i = 0; i < m_n; ++i )
    {
        if ( m_mesh[0]->getVertexColor( i ) != m_properties["maingl"].get( Fn::Property::D_COLOR ).value<QColor>() )
        {
            m_mesh[0]->setVertexColor( i, QColor( 0, 255, 0 ) );
            for ( int k = 0; k < m_n; ++k )
            {
                m_correlations->setValue( i, k, -1.0 );
            }
        }
    }
    Models::g()->submit();
}

void DatasetGlyphset::makeLittleBrains()
{
    for ( int i = 0; i < m_n; ++i )
    {
        if ( ( littleBrains[i] == NULL )
                && ( m_mesh[0]->getVertexColor( i ) != m_properties["maingl"].get( Fn::Property::D_COLOR ).value<QColor>() ) )
        {
            TriangleMesh2* mesh = new TriangleMesh2( m_mesh.at( properties( "maingl" ).get( Fn::Property::D_SURFACE_GLYPH_GEOMETRY ).toInt() ) );
            LittleBrainRenderer* m_renderer = new LittleBrainRenderer( mesh );
            m_renderer->init();

            littleBrains[i] = m_renderer;
            littleMeshes[i] = mesh;

            QMatrix4x4 sc;
            for ( int p = 0; p < m_n; ++p )
            {
                mesh->setVertexData( p, m_correlations->getValue( i, p ) );
            }
            QVector3D f1 = m_mesh.at( properties( "maingl" ).get( Fn::Property::D_SURFACE_GLYPH_GEOMETRY ).toInt() )->getVertex( i );
            shifts1[i] = f1;
            QVector3D f2 = m_mesh.at( properties( "maingl" ).get( Fn::Property::D_SURFACE ).toInt() )->getVertex( i );
            shifts2[i] = f2;
        }
        if ( ( littleBrains[i] != NULL )
                && ( m_mesh[0]->getVertexColor( i ) == m_properties["maingl"].get( Fn::Property::D_COLOR ).value<QColor>() ) )
        {
            delete littleBrains[i];
            delete littleMeshes[i];
            littleBrains[i] = NULL;
            littleMeshes[i] = NULL;
        }
    }
    Models::g()->submit();
}

void DatasetGlyphset::colorLittleBrains()
{
    QString fn = Models::g()->data( Models::g()->index( (int) Fn::Property::G_LAST_PATH, 0 ) ).toString();

    QString filter( "RGB color file (*.rgb)" );

    QFileDialog fd( NULL, "Open File", fn, filter );
    fd.setFileMode( QFileDialog::ExistingFile );
    fd.setAcceptMode( QFileDialog::AcceptOpen );
    if ( fd.exec() )
    {
        QStringList filenames = fd.selectedFiles();
        if ( filenames.length() > 0 )
        {
            QString filename = filenames.at( 0 );
            qDebug() << "loading: " << filename;
            QFile file( filename );
            if ( !file.open( QIODevice::ReadOnly ) )
            {
                qDebug() << "error reading: " << filename;
                return;
            }
            QTextStream in( &file );

            for ( int p = 0; p < m_n; p++ )
            {
                float r, g, b;
                in >> r >> g >> b;

                for ( int i = 0; i < m_n; i++ )
                {
                    if ( littleBrains[i] != NULL )
                    {
                        littleBrains[i]->beginUpdateColor();
                        littleBrains[i]->updateColor( p, r, g, b, 1.0 );
                        littleMeshes[i]->setVertexColor( p, r, g, b, 1.0 );
                        littleBrains[i]->endUpdateColor();
                    }
                }
            }
            file.close();
        }
    }
    else
    {
        qDebug() << "no file selected";
    }
    GLFunctions::reloadShaders();
    Models::g()->submit();
}

void DatasetGlyphset::deleteLittleBrains()
{
    for ( int i = 0; i < m_n; ++i )
    {
        delete littleBrains[i];
        delete littleMeshes[i];
        littleBrains[i] = NULL;
        littleMeshes[i] = NULL;
    }
    Models::g()->submit();
}

void DatasetGlyphset::draw( QMatrix4x4 pMatrix, QMatrix4x4 mvMatrix, int width, int height, int renderMode, QString target )
{
    if ( !properties( target ).get( Fn::Property::D_ACTIVE ).toBool() )
    {
        return;
    }

    if ( properties( target ).get( Fn::Property::D_DRAW_SURFACE ).toBool() )
    {
        int lr = properties( target ).get( Fn::Property::D_LEFT_RIGHT ).toInt();
        switch ( lr )
        {
            case 0:
                properties( target ).set( Fn::Property::D_START_INDEX, 0 );
                properties( target ).set( Fn::Property::D_END_INDEX, m_mesh[0]->numTris() );
                break;
            case 1:
                properties( target ).set( Fn::Property::D_START_INDEX, 0 );
                properties( target ).set( Fn::Property::D_END_INDEX, m_tris_middle - 1 );
                break;
            case 2:
                properties( target ).set( Fn::Property::D_START_INDEX, m_tris_middle );
                properties( target ).set( Fn::Property::D_END_INDEX, m_mesh[0]->numTris() );
                break;
            default:
                break;
        }
        DatasetCorrelation::draw( pMatrix, mvMatrix, width, height, renderMode, target );
    }

    QMatrix4x4 mvp = pMatrix * mvMatrix;

    if ( ( target == "maingl" ) && properties( target ).get( Fn::Property::D_LITTLE_BRAIN_VISIBILITY ).toBool() )
    {
        for ( int i = 0; i < m_n; ++i )
        {
            if ( littleBrains[i] != NULL )
            {
                QVector3D shift1 = shifts1[i];
                QVector3D shift2 = shifts2[i];
                QMatrix4x4 toOrigin;
                toOrigin.translate( shift2 );
                toOrigin.scale( properties( "maingl" ).get( Fn::Property::D_GLYPHRADIUS ).toFloat() );
                //Rotation of the individual glyphs:
                float rotx = 0;
                float roty = 0;
                float rotz = 0;
                if ( properties( "maingl" ).get( Fn::Property::D_GLYPH_ROTATION ).toBool() )
                {
                    rotx = properties( "maingl" ).get( Fn::Property::D_GLYPH_ROT_X ).toFloat();
                    roty = properties( "maingl" ).get( Fn::Property::D_GLYPH_ROT_Y ).toFloat();
                    rotz = properties( "maingl" ).get( Fn::Property::D_GLYPH_ROT_Z ).toFloat();
                }
                QMatrix4x4 rotMatrix;
                rotMatrix.rotate( rotx, 1, 0, 0 );
                rotMatrix.rotate( roty, 0, 1, 0 );
                rotMatrix.rotate( rotz, 0, 0, 1 );
                toOrigin *= rotMatrix;
                toOrigin.translate( -shift1 );
                QMatrix4x4 zshift;
                //little brain node towards the camera from big brain node...
                zshift.translate( 0, 0, 2 );

                QVector4D test = mvp * shift2;

                //1 is the viewport boundary, slightly larger value should prevent visible switching...
                float f = 1.2;
                if ( fabs( test.x() ) < f && fabs( test.y() ) < f )
                {
                    littleBrains[i]->m_LB_colormode = properties( "maingl" ).get( Fn::Property::D_LITTLE_BRAINS_COLORMODE ).toInt();
                    littleBrains[i]->draw( pMatrix, zshift * mvMatrix * toOrigin, width, height, renderMode, properties( target ) );
                }
                else
                {
                    //alternative (simpler) representation?
                }
            }
        }
    }

    int geoSurf = properties( "maingl" ).get( Fn::Property::D_SURFACE ).toInt();
    int geoGlyph = properties( "maingl" ).get( Fn::Property::D_SURFACE_GLYPH_GEOMETRY ).toInt();
    int geoCol = properties( "maingl" ).get( Fn::Property::D_SURFACE_GLYPH_COLOR ).toInt();
    int glyphstyle = properties( "maingl" ).get( Fn::Property::D_GLYPHSTYLE ).toInt();
    int colorMode = properties( "maingl" ).get( Fn::Property::D_GLYPH_COLORMODE ).toInt();

    float threshold = properties( "maingl" ).get( Fn::Property::D_THRESHOLD ).toFloat();
    float minlength = properties( "maingl" ).get( Fn::Property::D_MINLENGTH ).toFloat();
    int lr = properties( "maingl" ).get( Fn::Property::D_LEFT_RIGHT ).toInt();
    int sign = properties( "maingl" ).get( Fn::Property::D_GLYPH_THRESHOLD_SIGN ).toInt();

    glEnable( GL_BLEND );
    glPointSize( properties( "maingl" ).get( Fn::Property::D_PRIMSIZE ).toFloat() );
    glLineWidth( properties( "maingl" ).get( Fn::Property::D_PRIMSIZE ).toFloat() );

    if ( ( glyphstyle == 0 )
            && ( ( m_prenderer == 0 ) || ( prevGeo != geoSurf ) || ( prevGlyph != geoGlyph ) || ( prevCol != geoCol )
                    || ( prevGlyphstyle != glyphstyle ) || ( prevLR != lr ) || ( prevThreshSign != sign ) || glyphsChanged ) )
    {
        if ( !m_prenderer )
        {
            m_prenderer = new PointGlyphRenderer();
            m_prenderer->init();
        }
        makeCons();
        m_prenderer->initGeometry( consArray, consNumber );
        glyphsChanged = false;
    }

    if ( ( glyphstyle == 1 )
            && ( m_vrenderer == 0 || ( prevGeo != geoSurf ) || ( prevGlyph != geoGlyph ) || ( prevCol != geoCol ) || ( prevGlyphstyle != glyphstyle )
                    || ( prevLR != lr ) || ( prevThreshSign != sign )  || glyphsChanged ) )
    {
        if ( !m_vrenderer )
        {
            m_vrenderer = new VectorGlyphRenderer();
            m_vrenderer->init();
        }
        makeVecs();
        m_vrenderer->initGeometry( vecsArray, vecsNumber );
        glyphsChanged = false;
    }

    if ( ( glyphstyle == 2 )
            && ( m_pierenderer == 0 || ( prevGeo != geoSurf ) || ( prevGlyph != geoGlyph ) || ( prevCol != geoCol )
                    || ( prevGlyphstyle != glyphstyle ) || ( prevThresh != threshold ) || ( prevMinlength != minlength ) || ( prevLR != lr )
                    || ( prevColorMode != colorMode ) || ( prevThreshSign != sign )  || glyphsChanged ) )
    {
        if ( !m_pierenderer )
        {
            m_pierenderer = new PieGlyphRenderer();
            m_pierenderer->init();
        }
        makePies();
        m_pierenderer->initGeometry( pieArrays, numbers, maxNodeCount );
        glyphsChanged = false;
    }

    if ( ( glyphstyle == 3 )
            && ( ( m_dprenderer == 0 ) || ( prevGeo != geoSurf ) || ( prevGlyph != geoGlyph ) || ( prevCol != geoCol )
                    || ( prevGlyphstyle != glyphstyle ) || ( prevLR != lr ) || ( prevThreshSign != sign )  || glyphsChanged ) )
    {
        if ( !m_dprenderer )
        {
            m_dprenderer = new DiffPointGlyphRenderer();
            m_dprenderer->init();
        }
        makeDiffPoints();
        m_dprenderer->initGeometry( diffsArray, diffsNumber );
        glyphsChanged = false;
    }

    prevGeo = geoSurf;
    prevGlyph = geoGlyph;
    prevCol = geoCol;
    prevGlyphstyle = glyphstyle;
    prevThresh = threshold;
    prevMinlength = minlength;
    prevLR = lr;
    prevColorMode = colorMode;
    prevThreshSign = sign;

    if ( glyphstyle == 0 )
    {
        m_prenderer->draw( pMatrix, mvMatrix, width, height, renderMode, properties( target ) );
    }
    if ( glyphstyle == 1 )
    {
        m_vrenderer->draw( pMatrix, mvMatrix, width, height, renderMode, properties( target ) );
    }
    if ( glyphstyle == 2 )
    {
        m_pierenderer->draw( pMatrix, mvMatrix, width, height, renderMode, properties( target ) );
    }
    if ( glyphstyle == 3 )
    {
        m_dprenderer->draw( pMatrix, mvMatrix, width, height, renderMode, properties( target ) );
    }

    GLFunctions::drawColormapBar( properties( target ), width, height, renderMode );
}

bool DatasetGlyphset::filter( int i, int j, int lr, float threshold, int sign )
{
    float v = m_correlations->getValue( i, j );
    bool include = false;
    if ( ( sign == 0 || sign == 2 ) && ( v > threshold ) && ( v < m_maxThreshold ) && roi[i] && roi2[j] )
    {
        include = true;
    }
    if ( ( sign == 1 || sign == 2 ) && ( v < -threshold ) && ( v > -m_maxThreshold ) && roi[i] && roi2[j] )
    {
        include = true;
    }
    switch ( lr )
    {
        case 0:
            return include;
        case 1:
            if ( i < m_points_middle )
            {
                return include;
            }
            else
            {
                return false;
            }
        case 2:
            if ( i > m_points_middle )
            {
                return include;
            }
            else
            {
                return false;
            }
        default:
            return include;
    }
}

void DatasetGlyphset::makeCons()
{
    qDebug() << "making consArray: " << m_minThreshold << " m_maxThreshold: " << m_maxThreshold;
    if ( consArray )
        delete[] consArray;
    consArray = NULL;
    int geo = m_properties["maingl"].get( Fn::Property::D_SURFACE ).toInt();
    int glyph = m_properties["maingl"].get( Fn::Property::D_SURFACE_GLYPH_GEOMETRY ).toInt();
    int col = m_properties["maingl"].get( Fn::Property::D_SURFACE_GLYPH_COLOR ).toInt();

    int lr = m_properties["maingl"].get( Fn::Property::D_LEFT_RIGHT ).toInt();
    int sign = m_properties["maingl"].get( Fn::Property::D_GLYPH_THRESHOLD_SIGN ).toInt();

    m_n = m_mesh.at( geo )->numVerts();
    qDebug() << "nodes: " << m_n;
    consNumber = 0;
    //count first, then create array...
    for ( int i = 0; i < m_n; ++i )
    {
        if ( roi[i] )
        {
            for ( int j = 0; j < m_n; ++j )
            {
                if ( filter( i, j, lr, m_minThreshold, sign ) )
                {
                    ++consNumber;
                }
            }
        }
    }
    qDebug() << consNumber << " connections above threshold";
    int offset = 13;
    consArray = new float[offset * consNumber];
    consNumber = 0;
    for ( int i = 0; i < m_n; ++i )
    {
        if ( roi[i] )
        {
            for ( int j = 0; j < m_n; ++j )

            {
                float v = m_correlations->getValue( i, j );
                if ( filter( i, j, lr, m_minThreshold, sign ) )
                {
                    QVector3D f = m_mesh.at( geo )->getVertex( i );
                    QVector3D t = m_mesh.at( geo )->getVertex( j );

                    QVector3D fg = m_mesh.at( glyph )->getVertex( i );
                    QVector3D tg = m_mesh.at( glyph )->getVertex( j );
                    QVector3D dg = tg - fg;

                    QVector3D fc = m_mesh.at( col )->getVertex( i );
                    QVector3D tc = m_mesh.at( col )->getVertex( j );
                    QVector3D dc = tc - fc;

                    consArray[offset * consNumber] = f.x();
                    consArray[offset * consNumber + 1] = f.y();
                    consArray[offset * consNumber + 2] = f.z();
                    consArray[offset * consNumber + 3] = v;
                    consArray[offset * consNumber + 4] = t.x();
                    consArray[offset * consNumber + 5] = t.y();
                    consArray[offset * consNumber + 6] = t.z();

                    consArray[offset * consNumber + 7] = dg.x();
                    consArray[offset * consNumber + 8] = dg.y();
                    consArray[offset * consNumber + 9] = dg.z();

                    consArray[offset * consNumber + 10] = dc.x();
                    consArray[offset * consNumber + 11] = dc.y();
                    consArray[offset * consNumber + 12] = dc.z();

                    ++consNumber;
                }
            }
        }
    }
}

void DatasetGlyphset::makeDiffPoints()
{
    float diffMinThresh = m_minThreshold / 3.0;
    qDebug() << "making diffPoints: " << diffMinThresh << " m_maxThreshold: " << m_maxThreshold;
    if ( diffsArray )
        delete[] diffsArray;
    diffsArray = NULL;
    int geo = m_properties["maingl"].get( Fn::Property::D_SURFACE ).toInt();
    int glyph = m_properties["maingl"].get( Fn::Property::D_SURFACE_GLYPH_GEOMETRY ).toInt();
    int col = m_properties["maingl"].get( Fn::Property::D_SURFACE_GLYPH_COLOR ).toInt();

    int lr = m_properties["maingl"].get( Fn::Property::D_LEFT_RIGHT ).toInt();
    int sign = m_properties["maingl"].get( Fn::Property::D_GLYPH_THRESHOLD_SIGN ).toInt();

    m_n = m_mesh.at( geo )->numVerts();
    qDebug() << "nodes: " << m_n;
    int offset = 14;

    //for each triangle
    std::vector<unsigned int> tris = m_mesh.at( geo )->getTriangles();
    std::vector<int> idPairs;
    for ( unsigned int tri = 0; tri < tris.size(); tri += 3 )
    {
    // all three edges
        idPairs.push_back( tris.at( tri ) );
        idPairs.push_back( tris.at( tri + 1 ) );

        idPairs.push_back( tris.at( tri + 1 ) );
        idPairs.push_back( tris.at( tri + 2 ) );

        idPairs.push_back( tris.at( tri + 2 ) );
        idPairs.push_back( tris.at( tri ) );
    }
    qDebug() << "idPairs done, size: " << idPairs.size();
    diffsNumber = 0;
    for ( unsigned int idpair = 0; idpair < idPairs.size(); idpair += 2 )
    {
    //get two point ids i1,i2
        int i1 = idPairs.at( idpair );
        int i2 = idPairs.at( idpair + 1 );
    //if triangle on one side...
        if ( i1 > i2 )
        {
            for ( int j = 0; j < m_n; ++j )
            {
                //float v = qAbs( conn[i1][j] - conn[i2][j] );
                //if ( ( v > diffMinThresh ) && ( v < m_maxThreshold ) )
                if ( filter( i1, j, lr, m_minThreshold, sign ) && filter( i2, j, lr, m_minThreshold, sign ) )
                {
                    diffsNumber++;
                }
            }
        }
    }
    diffsArray = new float[offset * diffsNumber];
    qDebug() << "diffs: " << diffsNumber;
    diffsNumber = 0;
    for ( unsigned int idpair = 0; idpair < idPairs.size(); idpair += 2 )
    {
    //get two point ids i1,i2
        int i1 = idPairs.at( idpair );
        int i2 = idPairs.at( idpair + 1 );

        //if triangle on one side...
        if ( i1 > i2 )
        {
            for ( int j = 0; j < m_n; ++j )
            {
                //float v = qAbs( conn[i1][j] - conn[i2][j] );
                //if ( ( v > diffMinThresh ) && ( v < m_maxThreshold ) )
                float v1 = m_correlations->getValue( i1, j );
                float v2 = m_correlations->getValue( i2, j );
                if ( filter( i1, j, lr, m_minThreshold, sign ) && filter( i2, j, lr, m_minThreshold, sign ) )
                {
                    QVector3D f1 = m_mesh.at( geo )->getVertex( i1 );
                    QVector3D t = m_mesh.at( geo )->getVertex( j );

                    QVector3D fg1 = m_mesh.at( glyph )->getVertex( i1 );
                    QVector3D tg = m_mesh.at( glyph )->getVertex( j );
                    QVector3D dg1 = tg - fg1;

                    QVector3D fc1 = m_mesh.at( col )->getVertex( i1 );
                    QVector3D tc = m_mesh.at( col )->getVertex( j );
                    QVector3D dc1 = tc - fc1;

                    QVector3D f2 = m_mesh.at( geo )->getVertex( i2 );

                    QVector3D fg2 = m_mesh.at( glyph )->getVertex( i2 );
                    QVector3D dg2 = tg - fg2;

                    QVector3D fc2 = m_mesh.at( col )->getVertex( i2 );
                    QVector3D dc2 = tc - fc2;

                    diffsArray[offset * diffsNumber] = ( f1.x() + f2.x() ) / 2.0;
                    diffsArray[offset * diffsNumber + 1] = ( f1.y() + f2.y() ) / 2.0;
                    diffsArray[offset * diffsNumber + 2] = ( f1.z() + f2.z() ) / 2.0;
                    diffsArray[offset * diffsNumber + 3] = v1;
                    diffsArray[offset * diffsNumber + 4] = t.x();
                    diffsArray[offset * diffsNumber + 5] = t.y();
                    diffsArray[offset * diffsNumber + 6] = t.z();

                    diffsArray[offset * diffsNumber + 7] = ( dg1.x() + dg2.x() ) / 2.0;
                    diffsArray[offset * diffsNumber + 8] = ( dg1.y() + dg2.y() ) / 2.0;
                    diffsArray[offset * diffsNumber + 9] = ( dg1.z() + dg2.z() ) / 2.0;

                    diffsArray[offset * diffsNumber + 10] = ( dc1.x() + dc2.x() ) / 2.0;
                    diffsArray[offset * diffsNumber + 11] = ( dc1.y() + dc2.y() ) / 2.0;
                    diffsArray[offset * diffsNumber + 12] = ( dc1.z() + dc2.z() ) / 2.0;
                    diffsArray[offset * diffsNumber + 13] = v2;
                    diffsNumber++;
                }
            }
        }
    }
}

void DatasetGlyphset::makeVecs()
{
    qDebug() << "making vecsArray: " << m_minThreshold;
    if ( vecsArray )
        delete[] vecsArray;
    vecsArray = NULL;
    int geo = m_properties["maingl"].get( Fn::Property::D_SURFACE ).toInt();
    int glyph = m_properties["maingl"].get( Fn::Property::D_SURFACE_GLYPH_GEOMETRY ).toInt();
    int col = m_properties["maingl"].get( Fn::Property::D_SURFACE_GLYPH_COLOR ).toInt();

    int lr = m_properties["maingl"].get( Fn::Property::D_LEFT_RIGHT ).toInt();
    int sign = m_properties["maingl"].get( Fn::Property::D_GLYPH_THRESHOLD_SIGN ).toInt();

    m_n = m_mesh.at( geo )->numVerts();
    qDebug() << "nodes: " << m_n;
    vecsNumber = 0;
    //count first, then create array...
    for ( int i = 0; i < m_n; ++i )
    {
        if ( roi[i] )
        {
            for ( int j = 0; j < m_n; ++j )
            {
                if ( filter( i, j, lr, m_minThreshold, sign ) )
                {
                    ++vecsNumber;
                }
            }
        }
    }
    qDebug() << vecsNumber << " connections above threshold";
    int offset = 28;
    vecsArray = new float[offset * vecsNumber];
    vecsNumber = 0;
    for ( int i = 0; i < m_n; ++i )
    {
        if ( roi[i] )
        {
            for ( int j = 0; j < m_n; ++j )
            {
                float v = m_correlations->getValue( i, j );
                if ( filter( i, j, lr, m_minThreshold, sign ) )
                {
                    QVector3D f = m_mesh.at( geo )->getVertex( i );
                    QVector3D t = m_mesh.at( geo )->getVertex( j );

                    QVector3D fg = m_mesh.at( glyph )->getVertex( i );
                    QVector3D tg = m_mesh.at( glyph )->getVertex( j );
                    QVector3D dg = tg - fg;

                    QVector3D fc = m_mesh.at( col )->getVertex( i );
                    QVector3D tc = m_mesh.at( col )->getVertex( j );
                    QVector3D dc = tc - fc;

                    vecsArray[offset * vecsNumber] = f.x();
                    vecsArray[offset * vecsNumber + 1] = f.y();
                    vecsArray[offset * vecsNumber + 2] = f.z();
                    vecsArray[offset * vecsNumber + 3] = t.x();
                    vecsArray[offset * vecsNumber + 4] = t.y();
                    vecsArray[offset * vecsNumber + 5] = t.z();

                    vecsArray[offset * vecsNumber + 6] = v;
                    vecsArray[offset * vecsNumber + 7] = 1;

                    vecsArray[offset * vecsNumber + 8] = dg.x();
                    vecsArray[offset * vecsNumber + 9] = dg.y();
                    vecsArray[offset * vecsNumber + 10] = dg.z();

                    vecsArray[offset * vecsNumber + 11] = dc.x();
                    vecsArray[offset * vecsNumber + 12] = dc.y();
                    vecsArray[offset * vecsNumber + 13] = dc.z();

                    vecsArray[offset * vecsNumber + 14] = t.x();
                    vecsArray[offset * vecsNumber + 15] = t.y();
                    vecsArray[offset * vecsNumber + 16] = t.z();
                    vecsArray[offset * vecsNumber + 17] = f.x();
                    vecsArray[offset * vecsNumber + 18] = f.y();
                    vecsArray[offset * vecsNumber + 19] = f.z();

                    vecsArray[offset * vecsNumber + 20] = v;
                    vecsArray[offset * vecsNumber + 21] = -1;

                    vecsArray[offset * vecsNumber + 22] = -dg.x();
                    vecsArray[offset * vecsNumber + 23] = -dg.y();
                    vecsArray[offset * vecsNumber + 24] = -dg.z();

                    vecsArray[offset * vecsNumber + 25] = -dc.x();
                    vecsArray[offset * vecsNumber + 26] = -dc.y();
                    vecsArray[offset * vecsNumber + 27] = -dc.z();

                    ++vecsNumber;
                }
            }
        }
    }
}

bool edgeCompareHue( Connection* e1, Connection* e2 )
{
    QColor c1( e1->r * 255, e1->g * 255, e1->b * 255 );
    QColor c2( e2->r * 255, e2->g * 255, e2->b * 255 );
    return c1.hueF() > c2.hueF();
}

bool edgeCompareValue( Connection* e1, Connection* e2 )
{
    return e1->v > e2->v;
}

void DatasetGlyphset::makePies()
{

    qDebug() << "makePies begin";

    maxNodeCount = 0;

    float minlength = m_properties["maingl"].get( Fn::Property::D_MINLENGTH ).toFloat();
    float threshold = m_properties["maingl"].get( Fn::Property::D_THRESHOLD ).toFloat();
    if ( threshold < m_minThreshold )
        threshold = m_minThreshold;

    int lr = m_properties["maingl"].get( Fn::Property::D_LEFT_RIGHT ).toInt();
    int sign = m_properties["maingl"].get( Fn::Property::D_GLYPH_THRESHOLD_SIGN ).toInt();

    qDebug() << "threshold: " << threshold;

    if ( pieArrays )
    {
        for ( unsigned int i = 0; i < pieArrays->size(); ++i )
        {
            if ( pieArrays->at( i ) != NULL )
            {
                delete[] (float*) pieArrays->at( i );
            }
        }
        delete pieArrays;
        pieArrays = NULL;
        delete numbers;
        numbers = NULL;
    }

    qDebug() << "calcPies after deletion";

    int geo = m_properties["maingl"].get( Fn::Property::D_SURFACE ).toInt();
    int col = m_properties["maingl"].get( Fn::Property::D_SURFACE_GLYPH_COLOR ).toInt();

    m_n = m_mesh.at( geo )->numVerts();
    pieArrays = new std::vector<float*>( m_n, NULL );
    numbers = new std::vector<int>( m_n );

    //for all nodes in the current surface...
    //count first and throw super-threshold connections in sortable list, then create arrays...
    for ( int i = 0; i < m_n; ++i )
    {
        int count = 0;

        QList<Connection*> sortlist;
        if ( roi[i] )
        {
            for ( int j = 0; j < m_n; ++j )
            {
                if ( filter( i, j, lr, threshold, sign ) )
                {
                    QVector3D f = m_mesh.at( geo )->getVertex( i );
                    QVector3D t = m_mesh.at( geo )->getVertex( j );
                    QVector3D gdiff = t - f;

                    QVector3D fc = m_mesh.at( col )->getVertex( i );
                    QVector3D tc = m_mesh.at( col )->getVertex( j );
                    QVector3D dc = tc - fc;

                    if ( gdiff.length() > minlength )
                    {
                        sortlist.push_back( new Connection( f, dc, m_correlations->getValue( i, j ) ) );
                        ++count;
                    }
                }
            }
        }
        numbers->at( i ) = count;
        if ( count > maxNodeCount )
        {
            maxNodeCount = count;
        }
        //Magic!:
        int colorMode = m_properties["maingl"].get( Fn::Property::D_GLYPH_COLORMODE ).toInt();
        if ( colorMode == 0 )
        {
            qSort( sortlist.begin(), sortlist.end(), edgeCompareHue );
        }
        else
        {
            qSort( sortlist.begin(), sortlist.end(), edgeCompareValue );
        }

        int offset = 9;
        float* pieNodeArray = NULL;
        if ( count > 0 )
            pieNodeArray = new float[offset * count];
        for ( int nodecount = 0; nodecount < count; ++nodecount )
        {
            Connection* c = sortlist.at( nodecount );

            float v = c->v;

            int o = nodecount * offset;
            pieNodeArray[o] = c->fn.x();
            pieNodeArray[o + 1] = c->fn.y();
            pieNodeArray[o + 2] = c->fn.z();

            pieNodeArray[o + 3] = c->r;
            pieNodeArray[o + 4] = c->g;
            pieNodeArray[o + 5] = c->b;

            pieNodeArray[o + 6] = nodecount;
            pieNodeArray[o + 7] = count;
            pieNodeArray[o + 8] = v;
            delete c;
        }
        pieArrays->at( i ) = pieNodeArray;
    }
}

QList<Dataset*> DatasetGlyphset::createConnections()
{
    float threshold = m_properties["maingl"].get( Fn::Property::D_THRESHOLD ).toFloat();
    if ( threshold < m_minThreshold )
    {
        threshold = m_minThreshold;
    }
    int geo = m_properties["maingl"].get( Fn::Property::D_SURFACE ).toInt();
    m_n = m_mesh.at( geo )->numVerts();

    int lr = m_properties["maingl"].get( Fn::Property::D_LEFT_RIGHT ).toInt();
    int sign = m_properties["maingl"].get( Fn::Property::D_GLYPH_THRESHOLD_SIGN ).toInt();

    float minlength = m_properties["maingl"].get( Fn::Property::D_MINLENGTH ).toFloat();
    Connections* cons = new Connections();
    for ( int i = 0; i < m_n; ++i )
    {
        //if ( roi[i] )
        //{
            for ( int j = i + 1; j < m_n; ++j )
            {
                float v = m_correlations->getValue( i, j );
                if ( filter( i, j, lr, threshold, sign ) || filter( j, i, lr, threshold, sign ) )
                {
                    QVector3D f = m_mesh.at( geo )->getVertex( i );
                    QVector3D t = m_mesh.at( geo )->getVertex( j );

                    Edge* aedge = new Edge( f, t, v );

                    if ( aedge->length() > minlength )
                    {
                        cons->nodes << f;
                        cons->nodes << t;
                        cons->edges << aedge;
                    }
                }
            }
        //}
    }
    QList<Dataset*> l;
    if ( cons->edges.size() > 0 )
    {
        l.push_back( new DatasetCons( cons ) );
    }
    return l;
}

void DatasetGlyphset::setProperties()
{
    DatasetCorrelation::setProperties();
    m_properties["maingl"].createList( Fn::Property::D_SURFACE_GLYPH_GEOMETRY, m_displayList, 0, "glyphs" );
    m_properties["maingl"].createList( Fn::Property::D_SURFACE_GLYPH_COLOR, m_displayList, 0, "glyphs" );
    if ( m_is_split )
    {
        m_properties["maingl"].createList( Fn::Property::D_LEFT_RIGHT, { "both", "left", "right" }, 0, "general" );
    }
    littleBrains.clear();
    littleMeshes.clear();
    littleBrains.resize( m_n, NULL );
    littleMeshes.resize( m_n, NULL );
    shifts1.resize( m_n );
    shifts2.resize( m_n );
}

void DatasetGlyphset::applyROI()
{
    //color -> roi
    float* c = getMesh()->getVertexColors();
    for ( int i = 0; i < m_n; ++i )
    {
        //if color is not white, assume point is in ROI...
        if ( (c[i * 4] < 1.0) || (c[i * 4 + 1] < 1.0) || (c[i * 4 + 2] < 1.0) )
        {
            roi[i] = true;
        }
        else
        {
            roi[i] = false;
        }
    }
    glyphsChanged = true;
    m_correlations->makeHistogram( roi );
}

void DatasetGlyphset::switchGlyphsOff()
{
    m_properties["maingl"].set( Fn::Property::D_GLYPHSTYLE, 4 );
}

void DatasetGlyphset::loadROI( QString filename, bool* roin )
{
    qDebug() << "loading roi: " << filename;
    QFile file( filename );
    if ( !file.open( QIODevice::ReadOnly ) )
    {
        qDebug() << "something went wrong opening ROI file: " << filename;
        return;
    }
    QTextStream in( &file );
    if ( filename.endsWith( ".1D" ) )
    {
    //List of as many values as mesh nodes
        for ( unsigned int i = 0; i < m_mesh.at( 0 )->numVerts(); i++ )
        {
            float v;
            in >> v;
            if ( v > 0.5 )
            {
                roin[i] = true;
            }
            else
            {
                roin[i] = false;
            }
        }
    }
    else if ( filename.endsWith( ".rgb" ) )
    {
        //File with node rgb values
        for ( unsigned int i = 0; i < m_mesh.at( 0 )->numVerts(); i++ )
        {
            float r, g, b;
            in >> r >> g >> b;
            //if color is not white, assume point is in ROI...
            if ( (r < 1.0) || (g < 1.0) || (b < 1.0) )
            {
                roin[i] = true;
            }
            else
            {
                roin[i] = false;
            }
        }
    }
    else
    {
    //File with node ids
        std::vector<unsigned int> ids;
        while ( !in.atEnd() )
        {
            QString line = in.readLine();
            QStringList sl = line.split( " " );
            ids.push_back( sl.at( 0 ).toInt() );
        }

        for ( unsigned int i = 0; i < m_mesh.at( 0 )->numVerts(); i++ )
        {
            roin[i] = false;
            for ( unsigned int k = 0; k < ids.size(); ++k )
            {
                if ( ids[k] == i )
                {
                    roi2[i] = true;
                }
            }
        }
    }
    file.close();
}

void DatasetGlyphset::initROI()
{
    roi = new bool[m_mesh.at( 0 )->numVerts()];
    roi2 = new bool[m_mesh.at( 0 )->numVerts()];
    for ( unsigned int i = 0; i < m_mesh.at( 0 )->numVerts(); i++ )
    {
        roi[i] = false;
        roi2[i] = true;
    }
}

inline uint qHash( const QColor color )
{
    return color.red() + color.green() + color.blue();
}

void DatasetGlyphset::exportColors()
{
    QString filename = QFileDialog::getSaveFileName( NULL, "save 1D file", m_colors_name + ".col_" );

    QSet<QColor>* colors = new QSet<QColor>();
    for ( unsigned int i = 0; i < m_mesh.at( prevGeo )->numVerts(); i++ )
    {
        QColor vcolor = m_mesh.at( prevGeo )->getVertexColor( i );
        QColor erasecolor = m_properties["maingl"].get( Fn::Property::D_COLOR ).value<QColor>();
        if ( vcolor != erasecolor )
        {
            colors->insert( vcolor );
        }
    }
    qDebug() << "writing: " << colors->size() << " pairs of 1D-files...";
    QList<QColor> color_list = colors->toList();
    for ( int fn = 0; fn < color_list.size(); ++fn )
    {
        QColor c = color_list.at( fn );
        QFile file( filename + QString::number( c.red() ) + "_" + QString::number( c.green() ) + "_" + QString::number( c.blue() ) + ".1D.roi" );
        QFile file2( filename + QString::number( c.red() ) + "_" + QString::number( c.green() ) + "_" + QString::number( c.blue() ) + ".1D" );
        if ( !file.open( QIODevice::WriteOnly ) )
        {
            qDebug() << "file open failed: " << filename;
            return;
        }
        if ( !file2.open( QIODevice::WriteOnly ) )
        {
            qDebug() << "file open failed: " << filename;
            return;
        }
        QTextStream out( &file );
        QTextStream out2( &file2 );
        for ( unsigned int i = 0; i < m_mesh.at( prevGeo )->numVerts(); i++ )
        {
            QColor vcolor = m_mesh.at( prevGeo )->getVertexColor( i );
            if ( vcolor == c )
            {
                out << i << " " << fn + 1 << endl;
                out2 << 1.0 << endl;
            }
            else
            {
                out2 << 0.0 << endl;
            }
        }
        file.close();
        file2.close();
    }
}

void DatasetGlyphset::avgCon()
{
    for ( int i = 0; i < m_n; ++i )
    {
        double v = 0;
        int nroi = 0;
        for ( int r = 0; r < m_n; ++r )
        {
            if ( roi[r] )
            {
                if ( r == i )
                {
                    v += 1;
                }
                else
                {
                    v += m_correlations->getValue( r, i );
                }
                ++nroi;
            }
        }
        for ( unsigned int m = 0; m < m_mesh.size(); m++ )
        {
            m_mesh[m]->setVertexData( i, v / (double) nroi );
        }
    }
    Models::g()->submit();
}

void DatasetGlyphset::avgConRtoZ()
{
    for ( int i = 0; i < m_n; ++i )
    {
        double v = 0;
        int nroi = 0;
        for ( int r = 0; r < m_n; ++r )
        {
            if ( roi[r] )
            {
                if ( r == i )
                {
                    //v += 1;
                }
                else
                {
                    double v1 = m_correlations->getValue( r, i );
                    double z = 0.5 * qLn( ( 1 + v1 ) / ( 1 - v1 ) );
                    v += z;
                }
                ++nroi;
            }
        }
        for ( unsigned int m = 0; m < m_mesh.size(); m++ )
        {
            double v1 = v / (double) nroi;
            m_mesh[m]->setVertexData( i, ( qExp( 2 * v1 ) - 1 ) / ( qExp( 2 * v1 ) + 1 ) );
        }
    }
    Models::g()->submit();
}

void DatasetGlyphset::slotCopyColors()
{
    int n = properties( "maingl" ).get( Fn::Property::D_SURFACE ).toInt();
    TriangleMesh2* mesh = m_mesh[n];

    QColor color;

    float selectedMin = properties( "maingl" ).get( Fn::Property::D_SELECTED_MIN ).toFloat();
    float selectedMax = properties( "maingl" ).get( Fn::Property::D_SELECTED_MAX ).toFloat();

    for ( int i = 0; i < m_n; ++i )
    {
        ColormapBase cmap = ColormapFunctions::getColormap( properties( "maingl" ).get( Fn::Property::D_COLORMAP ).toInt() );

        float value = ( m_correlations->getValue( m_prevPickedID, i ) - selectedMin ) / ( selectedMax - selectedMin );
        color = cmap.getColor( qMax( 0.0f, qMin( 1.0f, value ) ) );

        mesh->setVertexColor( i, color );
    }
}

void DatasetGlyphset::thresholdChanged( QVariant v )
{
    float f = v.toFloat();
    m_properties["maingl"].set( Fn::Property::D_THRESHOLD_PERC, m_correlations->percFromThresh( f ) );
}

void DatasetGlyphset::thresholdPercChanged( QVariant v )
{
    float f = v.toFloat();
    m_properties["maingl"].set( Fn::Property::D_THRESHOLD, m_correlations->threshFromPerc( f ) );
}
