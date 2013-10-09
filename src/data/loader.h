/*
 * Loader.h
 *
 * Created on: May 3, 2012
 * @author Ralph Schurade
 */

#ifndef LOADER_H_
#define LOADER_H_

#include "datasets/dataset.h"
#include "datasets/datasetmesh.h"

#include "../thirdparty/nifti/nifti1_io.h"

#include <QDir>
#include <QVector>

class Loader
{
public:
    Loader(Dataset* selected = NULL);
    virtual ~Loader();

    bool load();

    Dataset* getDataset( int id = 0 );
    int getNumDatasets();

    void setFilename( QDir fn );
    bool succes();
private:
    Fn::DatasetType determineType();

    bool loadNifti();
    bool loadNiftiHeader( QString hdrPath );

    bool loadNiftiScalar( QString fileName );
    bool loadNiftiVector3D( QString fileName );
    bool loadNiftiTensor( QString fileName );
    bool loadNiftiQBall( QString fileName );
    bool loadNiftiBingham( QString fileName );
    bool loadNiftiFMRI( QString fileName );
    bool loadNiftiDWI( QString fileName );
    bool loadNiftiDWI_FNAV2( QString fileName );
    QVector<float> loadBvals( QString fileName );
    QVector<QVector3D> loadBvecs( QString fileName, QVector<float> bvals );

    bool loadVTK();
    bool loadASC( QVector3D offset = QVector3D(0,0,0) );
    bool loadSet();
    bool loadGlyphset();
    bool loadCons();
    bool loadMEG();
    bool loadTree();
    bool loadRGB();
    bool load1D();

    nifti_image* m_header;
    QDir m_fileName;

    Fn::DatasetType m_datasetType;
    QVector<Dataset*> m_dataset;
    Dataset* m_selectedDataset;

    bool m_success;
};

#endif /* LOADER_H_ */
