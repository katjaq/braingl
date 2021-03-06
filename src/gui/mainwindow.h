/*
 * mainwindow.h
 *
 * Created on: May 03, 2012
 * @author Ralph Schurade
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../data/enums.h"

#include <QAbstractItemModel>
#include <QFileDialog>
#include <QMainWindow>
#include <QSettings>
#include <QTcpServer>

class ColormapEditWidget;
class Dataset;
class DatasetListWidget;
class FNDockWidget;
class GLWidget;
class ROIArea;
class ROIWidget;
class ScriptWidget;
class ShaderEditWidget;
class ToolBar;

class QAction;
class QListView;
class QMenu;
class QTabWidget;
class QTableView;

class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    MainWindow( bool debug = false, bool resetSettings = false );

    void closeEvent( QCloseEvent *event );
    bool load( QString fileName );
    bool loadAndAlgo( QString fileName, QString algo, QVariant param );
    bool loadRoi( QString fileName );

    void saveScene( QString fileName, bool packAndGo );
    void loadScene( QString fileName );
    void loadSceneDeprecated( QString fileName );

private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createDockWindows();

    bool save( Dataset* ds );
    void saveDataset( Dataset* ds, QString filter = "" );
    void saveRoi( ROIArea* roi );

    void saveSettings();
    void loadSettings();
    void loadSetting( QSettings &settings, Fn::Property setting );
    void loadColormaps( bool resetSettings );

    void setCurrentFile( const QString &fileName );
    void updateRecentFileActions();
    QString strippedName( const QString &fullFileName );

    void setCurrentScene( const QString &fileName );
    void updateRecentSceneActions();

    bool load( QString fileName, QList<QVariant> state );

    Dataset* selectedDataset();

    bool m_debug;

    QMainWindow* m_centralWidget;

    GLWidget* mainGLWidget;
    GLWidget* mainGLWidget2;
    DatasetListWidget* m_datasetWidget;
    ROIWidget* m_roiWidget;
    ShaderEditWidget* m_shaderEditWidget;
    ScriptWidget* m_scriptWidget;
    FNDockWidget* m_dockDSP;
    ColormapEditWidget* m_colormapEditWidget;

    FNDockWidget* m_dockHTW;
    FNDockWidget* m_dockHTW2;

    QMenu* fileMenu;
    QMenu* optionMenu;
    QMenu* viewMenu;
    QMenu* tabMenu;
    QMenu* helpMenu;
    QMenu *recentFilesMenu;
    QMenu *recentScenesMenu;

    QToolBar* fileToolBar;
    QToolBar* editToolBar;
    ToolBar* m_toolsToolBar;

    QTcpServer* m_tcpServer;
    QTcpSocket* m_clientConnection;

    enum
    {
        MaxRecentFiles = 15
    };
    QAction *recentFileActs[MaxRecentFiles];
    QAction *separatorAct;
    QString curFile;

    QAction *recentSceneActs[MaxRecentFiles];
    QString curScene;

    QAction* newAct;
    QAction* openAct;
    QAction* saveAct;
    QAction* quitAct;

    QAction* loadRoiAct;
    QAction* saveSceneAct;
    QAction* packAndGoAct;

    QAction* exportColormapsAct;
    QAction* importColormapsAct;

    QAction* newMainGLAct;

    QAction* lockDockTitlesAct;
    QAction* renderCrosshairsAct;

    QAction* newPlaneAct;
    QAction* newGuideAct;
    QAction* newLabelAct;

    QAction* showAxialAct;
    QAction* showCoronalAct;
    QAction* showSagittalAct;

    QAction* screenshotAct;
    QAction* resetSettingsAct;
    QAction* aboutAct;

    QAction* toggleShaderEditAct;

    QAction* standardViewAxialAct;
    QAction* standardViewCoronalAct;
    QAction* standardViewSagittalAct;

    QAction* newSelectionBoxAct;
    QAction* continousRenderingAct;

    QAction* dilbertAct;

    QAction* fullScreenAct;

    QFileDialog* fd;

    static int countMainGL;

    Qt::WindowFlags m_dockFlags;
    QString m_styleSheet;
    QRect m_geometry;

public slots:
    void screenshot( bool exitAfter = false );
    void runScript();

private slots:
    void slotNew();
    void open();
    void openRecentFile();
    void openRecentScene();
    void save();
    void loadRoi();
    void saveScene();
    void packAndGo();
    void exportColormaps();
    void importColormaps();
    void about();
    void resetSettings();
    void newMainGL();
    void continousRendering();

    void slotToggleAxialSlice();
    void slotToggleCoronalSlice();
    void slotToggleSagittalSlice();

    void slotStandardAxialView();
    void slotStandardCoronalView();
    void slotStandardSagittalView();
    void slotToggleShaderEdit();

    void slotDilbert();

    void slotRenderCrosshairs( bool value );

    void slotDatasetSelectionChanged();

    void saveFilterChanged( QString filterString );

    void newPlane();
    void newGuide();
    void newLabel();


    void onKeyPressed( int key, Qt::KeyboardModifiers mods );
    void toggleFullScreen();

    void receiveTCP();
    void readTCP();
};

#endif
