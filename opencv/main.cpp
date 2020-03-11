

/*TUTORIAL DE USO PARA HACER UNA PRUEBA ANTES DE TOCAR NADA Y COMPROBAR QUE FUNCIONA:
 *
 *
 * 1- EN LA LINEA 184, poner la ruta absoluta del video. El video viene incluido, se llama "1.mp4"
 * 2- Antes habian mas pasos, pero me he cargado lo de los parametros para que no sea un coñazo hacer pruebas,
 *    con ejecutar deberia funcionarte :)
 *
 *
 *    *IMPORTANTE* : Si no te funciona o incluye bien openCV, NO TRASTEES NADA!!!!
 *                   Lo unico que tienes que hacer es desinstalar el que tienes,
 *                   ir a "añadir/quitar software", activar los repositorios AUR,
 *                   instalar OPENCV2, OPENCV2-OPT y OPENCV2-OPT-SAMPLES.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <opencv2/opencv.hpp>

using std::cout;
using std::endl;

//Resolucion Ancho y alto (por defecto de la ventana, luego se cambia por la resolucion del video)
int window_width  = 640;
int window_height = 480;

// Frame counting and limiting
int    frame_count = 0; //Frame inicial de reproduccion
double frame_start_time, frame_end_time, frame_draw_time;

/* FUNCIONES QUE NOS VAN A HACER FALTA PARA REPRODUCIR VIDEO.
 *          NOS VAN A HACER FALTA PERO PREFIERO DEJAR LOS COMENTARIOS ORIGINALES
 *          PORQUE NO ENTIENDO MUCHO... :((
 *
 *          Supongo que lo suyo es hacerse un .hpp en "/util" con estas superfunciones
 *          y hacer sus llamaditas (Explicadas en el main)
 *
 */

// Function turn a cv::Mat into a texture, and return the texture ID as a GLuint for use
static GLuint matToTexture(const cv::Mat &mat, GLenum minFilter, GLenum magFilter, GLenum wrapFilter) {
    // Generate a number for our textureID's unique handle
    GLuint textureID;
    glGenTextures(1, &textureID);

    // Bind to our texture handle
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Catch silly-mistake texture interpolation method for magnification
    if (magFilter == GL_LINEAR_MIPMAP_LINEAR  ||
        magFilter == GL_LINEAR_MIPMAP_NEAREST ||
        magFilter == GL_NEAREST_MIPMAP_LINEAR ||
        magFilter == GL_NEAREST_MIPMAP_NEAREST)
    {
        cout << "You can't use MIPMAPs for magnification - setting filter to GL_LINEAR" << endl;
        magFilter = GL_LINEAR;
    }

    // Set texture interpolation methods for minification and magnification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

    // Set texture clamping method
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapFilter);

    // Set incoming texture format to:
    // GL_BGR       for CV_CAP_OPENNI_BGR_IMAGE,
    // GL_LUMINANCE for CV_CAP_OPENNI_DISPARITY_MAP,
    // Work out other mappings as required ( there's a list in comments in main() )
    GLenum inputColourFormat = GL_BGR;
    if (mat.channels() == 1)
    {
        inputColourFormat = GL_LUMINANCE;
    }

    // Create the texture
    glTexImage2D(GL_TEXTURE_2D,     // Type of texture
                 0,                 // Pyramid level (for mip-mapping) - 0 is the top level
                 GL_RGB,            // Internal colour format to convert to
                 mat.cols,          // Image width  i.e. 640 for Kinect in standard mode
                 mat.rows,          // Image height i.e. 480 for Kinect in standard mode
                 0,                 // Border width in pixels (can either be 1 or 0)
                 inputColourFormat, // Input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
                 GL_UNSIGNED_BYTE,  // Image data type
                 mat.ptr());        // The actual image data itself

    // If we're using mipmaps then generate them. Note: This requires OpenGL 3.0 or higher
    if (minFilter == GL_LINEAR_MIPMAP_LINEAR  ||
        minFilter == GL_LINEAR_MIPMAP_NEAREST ||
        minFilter == GL_NEAREST_MIPMAP_LINEAR ||
        minFilter == GL_NEAREST_MIPMAP_NEAREST)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    return textureID;
}

static void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

static void resize_callback(GLFWwindow* window, int new_width, int new_height) {
    glViewport(0, 0, window_width = new_width, window_height = new_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, window_width, window_height, 0.0, 0.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

static void draw_frame(const cv::Mat& frame) {
    // Clear color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);     // Operate on model-view matrix

    glEnable(GL_TEXTURE_2D);
    GLuint image_tex = matToTexture(frame, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP);

    /* Draw a quad */
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0); glVertex2i(0,   0);
    glTexCoord2i(0, 1); glVertex2i(0,   window_height);
    glTexCoord2i(1, 1); glVertex2i(window_width, window_height);
    glTexCoord2i(1, 0); glVertex2i(window_width, 0);
    glEnd();

    glDeleteTextures(1, &image_tex);
    glDisable(GL_TEXTURE_2D);
}

void lock_frame_rate(double frame_rate) {
    static double allowed_frame_time = 1.0 / frame_rate;

    // Note: frame_start_time is called first thing in the main loop
    frame_end_time = glfwGetTime();  // in seconds

    frame_draw_time = frame_end_time - frame_start_time;

    double sleep_time = 0.0;

    if (frame_draw_time < allowed_frame_time) {
        sleep_time = allowed_frame_time - frame_draw_time;
        usleep(1000000 * sleep_time);
    }

    // Debug stuff
    double potential_fps = 1.0 / frame_draw_time;
    double locked_fps    = 1.0 / (glfwGetTime() - frame_start_time);
    cout << "Frame [" << frame_count << "] ";
    cout << "Draw: " << frame_draw_time << " Sleep: " << sleep_time;
    cout << " Pot. FPS: " << potential_fps << " Locked FPS: " << locked_fps << endl;
}

static void init_opengl(int w, int h) {
    glViewport(0, 0, w, h); // use a screen size of WIDTH x HEIGHT

    glMatrixMode(GL_PROJECTION);     // Make a simple 2D projection on the entire window
    glLoadIdentity();
    glOrtho(0.0, w, h, 0.0, 0.0, 100.0);

    glMatrixMode(GL_MODELVIEW);    // Set the matrix mode to object modeling

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the window
}



//MAIN --> Aqui estan las llamadas y las movidas que nosotros tendremos que hacer
int main() {

    cv::VideoCapture capture("/home/juanma/CLionProjects/opencv/1.mp4"); //Capturamos el video pasandole su ruta

    if (!capture.isOpened()) {
        cout << "Algo ha fallao abriendo el video hermano jaja" << endl;
        exit(EXIT_FAILURE);
    }

    double fps = 0.0;
    fps = capture.get(CV_CAP_PROP_FPS); //Obtenemos los fps nativos del video
    if (fps != fps) { // Si por cualquier cosa no podemos obtener esa informacion, pues le decimos nosotros los fps a los que va a ir
        fps = 30.0;
    }

    cout << "FPS: " << fps << endl;

    //Ponemos por defecto la resolucion nativa del video
    window_width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
    window_height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
    cout << "Video width: " << window_width << endl;
    cout << "Video height: " << window_height << endl;

    //INICIAMOS UNA ventana nueva de glfw
    GLFWwindow* window;
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    //Le he indicado OpenGL en su version 3.0 porque me funciona sin tener glad metido ni movidas.
    //Si se le integra Glad, pues supongo que habrá que meter la versión 4.5 (PARA QUE ME FUNCIONE A MI TAMBIEN EN EL
    // PORTATIL PONER 4.5, NO 4.6)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    //Creamos una ventana con glfw
    window = glfwCreateWindow(window_width, window_height, "Cyborgeddon", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowSizeCallback(window, resize_callback); //Redimensionamos la ventana por si el video no tiene la dimension
                                                        //Que la ventana tiene por defecto.

    glfwMakeContextCurrent(window);

    glfwSwapInterval(1); //Indicamos la cantidad de frames que se cambian por cada iteración (ponemos 1 obviamente)


    GLenum err = glewInit(); //  Initialise glew (must occur AFTER window creation or glew will error)
    if (GLEW_OK != err)
    {
        cout << "GLEW initialisation error: " << glewGetErrorString(err) << endl;
        exit(-1);
    }
    cout << "GLEW okay - using version: " << glewGetString(GLEW_VERSION) << endl;

    init_opengl(window_width, window_height);

    double video_start_time = glfwGetTime(); //Momento de inicio del video
    double video_end_time = 0.0; //Momento de fin de video, se lo indicamos más bajo

    cv::Mat frame;
    while (!glfwWindowShouldClose(window)) { //Bucle de reproduccion
        frame_start_time = glfwGetTime();
        if (!capture.read(frame)) {
            cout << "No hemos podido leer mas frames. Probablemente el video haya terminado." << endl;
            break;
        }

        draw_frame(frame); //Pintamos en la ventana
        video_end_time = glfwGetTime();

        glfwSwapBuffers(window); //Cargamos el siguiente fotograma
        glfwPollEvents();

        ++frame_count;
        lock_frame_rate(fps); //Bloqueamos el framerate (En la medida de lo posible, nunca es 100% exacto.)
    }

    cout << "Duracion total del video: " << video_end_time - video_start_time << " seconds" << endl;

    glfwDestroyWindow(window); //Nos cargamos la ventana
    glfwTerminate(); //Chapamos

    exit(EXIT_SUCCESS);
}