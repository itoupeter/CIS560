#include "mygl.h"
#include <la.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <QXmlStreamReader>
#include <QFileDialog>
#include <renderthread.h>
#include <raytracing/samplers/stratifiedpixelsampler.h>
#include <scene/bvh.h>
#include <openGL/canvas.h>
#include <QElapsedTimer>
#include <vector>
using std::vector;

MyGL::MyGL(QWidget *parent)
    : GLWidget277(parent)
{
    setFocusPolicy(Qt::ClickFocus);
}

MyGL::~MyGL()
{
    makeCurrent();

    vao.destroy();
}

void MyGL::initializeGL()
{
    // Create an OpenGL context
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    // Set the size with which points should be rendered
    glPointSize(5);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.5, 0.5, 0.5, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    vao.create();

    // Create and set up the diffuse shader
    prog_lambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat-color shader
    prog_flat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    vao.bind();

    //Test scene data initialization
    scene.CreateTestScene();
    integrator.scene = &scene;
    integrator.intersection_engine = &intersection_engine;
    intersection_engine.scene = &scene;
    ResizeToSceneCamera();
}

void MyGL::resizeGL(int w, int h)
{
    gl_camera = Camera(w, h);

    glm::mat4 viewproj = gl_camera.getViewProj();

    // Upload the projection matrix
    prog_lambert.setViewProjMatrix(viewproj);
    prog_flat.setViewProjMatrix(viewproj);

    printGLErrorLog();
}

// This function is called by Qt any time your GL window is supposed to update
// For example, when the function updateGL is called, paintGL is called implicitly.
void MyGL::paintGL()
{
    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#define PROGRESSIVE

#ifdef PROGRESSIVE
    static Canvas canvas;

    canvas.draw( this );
#else
    // Update the viewproj matrix
    prog_lambert.setViewProjMatrix(gl_camera.getViewProj());
    prog_flat.setViewProjMatrix(gl_camera.getViewProj());
    GLDrawScene();
#endif
}

void MyGL::GLDrawScene()
{
    //---draw scene geometries---
    for(Geometry *g : scene.objects)
    {
        if(g->drawMode() == GL_TRIANGLES)
        {
            prog_lambert.setModelMatrix(g->transform.T());
            prog_lambert.draw(*this, *g);
        }
        else if(g->drawMode() == GL_LINES)
        {
            prog_flat.setModelMatrix(g->transform.T());
            prog_flat.draw(*this, *g);
        }
    }

    //---draw scene lights---
    for(Geometry *l : scene.lights)
    {
        prog_flat.setModelMatrix(l->transform.T());
        prog_flat.draw(*this, *l);
    }
    prog_flat.setModelMatrix(glm::mat4(1.0f));
    prog_flat.draw(*this, scene.camera);

    //---draw bounding box for scene geometries---
    for( BoundingBox *pBBox : Scene::allBBoxes ){
        prog_flat.setModelMatrix( glm::mat4( 1.f ) );
        prog_flat.draw( *this, *pBBox );
    }
}

void MyGL::ResizeToSceneCamera()
{
    this->setFixedWidth(scene.camera.width);
    this->setFixedHeight(scene.camera.height);
    gl_camera = Camera(scene.camera);
}

void MyGL::keyPressEvent(QKeyEvent *e)
{
    float amount = 2.0f;
    if(e->modifiers() & Qt::ShiftModifier){
        amount = 10.0f;
    }
    // http://doc.qt.io/qt-5/qt.html#Key-enum
    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_Right) {
        gl_camera.RotateAboutUp(-amount);
    } else if (e->key() == Qt::Key_Left) {
        gl_camera.RotateAboutUp(amount);
    } else if (e->key() == Qt::Key_Up) {
        gl_camera.RotateAboutRight(-amount);
    } else if (e->key() == Qt::Key_Down) {
        gl_camera.RotateAboutRight(amount);
    } else if (e->key() == Qt::Key_1) {
        gl_camera.fovy += amount;
    } else if (e->key() == Qt::Key_2) {
        gl_camera.fovy -= amount;
    } else if (e->key() == Qt::Key_W) {
        gl_camera.TranslateAlongLook(amount);
    } else if (e->key() == Qt::Key_S) {
        gl_camera.TranslateAlongLook(-amount);
    } else if (e->key() == Qt::Key_D) {
        gl_camera.TranslateAlongRight(amount);
    } else if (e->key() == Qt::Key_A) {
        gl_camera.TranslateAlongRight(-amount);
    } else if (e->key() == Qt::Key_Q) {
        gl_camera.TranslateAlongUp(-amount);
    } else if (e->key() == Qt::Key_E) {
        gl_camera.TranslateAlongUp(amount);
    } else if (e->key() == Qt::Key_F) {
        gl_camera.CopyAttributes(scene.camera);
    } else if (e->key() == Qt::Key_R) {
        scene.camera = Camera(gl_camera);
        scene.camera.recreate();
    }
    gl_camera.RecomputeAttributes();
    update();  // Calls paintGL, among other things
}

void MyGL::SceneLoadDialog()
{
//    QString filepath = QFileDialog::getOpenFileName(0, QString("Load Scene"), QString("../scene_files"), tr("*.xml"));
    QString filepath = "../veach_mis.xml";
    if(filepath.length() == 0)
    {
        return;
    }

    QFile file(filepath);
    int i = filepath.length() - 1;
    while(QString::compare(filepath.at(i), QChar('/')) != 0)
    {
        i--;
    }
    QStringRef local_path = filepath.leftRef(i+1);
    //Reset all of our objects
    scene.Clear();
    integrator = Integrator();
    intersection_engine = IntersectionEngine();
    //Load new objects based on the XML file chosen.
    xml_reader.LoadSceneFromFile(file, local_path, scene, integrator);
    integrator.scene = &scene;
    integrator.intersection_engine = &intersection_engine;
    intersection_engine.scene = &scene;
    //---BVH---
    for( Geometry *pGeometry : scene.objects ){
        pGeometry->computeBounds();
    }
    BVH::scene = &scene;
    BVH::clear( intersection_engine.root );
    intersection_engine.root = BVH::build( scene.objects, intersection_engine.root, 0 );

    ResizeToSceneCamera();
    update();

}

void MyGL::RaytraceScene()
{
    QString filepath = QFileDialog::getSaveFileName(0, QString("Save Image"), QString("../rendered_images"), tr("*.bmp"));
    if(filepath.length() == 0)
    {
        return;
    }

    //---timer---
    QElapsedTimer timer;
    timer.start();

#define MULTITHREADED
#ifdef MULTITHREADED
    int width( scene.camera.width );
    int height( scene.camera.height );
    int num_render_threads( 4 );
    RenderThread **render_threads = new RenderThread*[num_render_threads];

    //---initialize pixel coordinates array---
    std::default_random_engine generator0( time( 0 ) );
    std::uniform_real_distribution< double > distribution0( 0., .999 );
    QList< int > &pixel_coords = RenderThread::pixel_coords;
    int nCoords( 0 );

    pixel_coords.clear();
    for( int i = 0; i < height; ++i ){
        for( int j = 0; j < width; ++j ){
            pixel_coords.push_back( ( i << 16 ) | j );
            ++nCoords;

            int a( nCoords * distribution0( generator0 ) );
            int b( nCoords - 1 );
            int tmp( pixel_coords[ a ] );

            pixel_coords[ a ] = pixel_coords[ b ];
            pixel_coords[ b ] = tmp;
        }
    }

    //---launch thread---
    for( int i = 0; i < num_render_threads; ++i ){
        render_threads[ i ] = new RenderThread( scene.sqrt_samples, 5, &(scene.film), &(scene.camera), &(integrator) );
        render_threads[ i ]->start();
    }

    //---display image progressively---
    bool still_running;
    do
    {
        still_running = false;

        for(unsigned int i = 0; i < num_render_threads; i++)
        {
            if(render_threads[i]->isRunning())
            {
                still_running = true;

                static int flag( 0 );
                if( flag++ > 999 ){
                    flag = 0;
                    scene.film.WriteImage(filepath);
                    update();
                }

                break;
            }
        }
        if(still_running)
        {
            //Free the CPU to let the remaining render threads use it
            QThread::yieldCurrentThread();
        }
    }
    while(still_running);

    //Finally, clean up the render thread objects
    for(unsigned int i = 0; i < num_render_threads; i++)
    {
        delete render_threads[i];
    }
    delete [] render_threads;

#else
    StratifiedPixelSampler pixel_sampler(scene.sqrt_samples);
    for(unsigned int i = 0; i < scene.camera.width; i++)
    {
        for(unsigned int j = 0; j < scene.camera.height; j++)
        {
            QList<glm::vec2> sample_points = pixel_sampler->GetSamples(i, j);
            glm::vec3 accum_color;
            for(int a = 0; a < sample_points.size(); a++)
            {
                glm::vec3 color = integrator.TraceRay(scene.camera.Raycast(sample_points[a]), 0);
                accum_color += color;
            }
            scene.film.pixels[i][j] = accum_color / (float)sample_points.size();
        }
    }
#endif
    scene.film.WriteImage(filepath);
    std::cout << "\n"
              << "----OK!----\n"
              << "Time Elapsed: " << timer.elapsed()
              << std::endl;
}
