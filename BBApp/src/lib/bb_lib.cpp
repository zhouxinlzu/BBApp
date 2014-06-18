#include "bb_lib.h"
#include "../model/trace.h"

#include <iostream>

#include <QSize>
#include <QVector>
#include <QStandardPaths>
#include <QDir>
#include <QImage>

char *persist_vs =
        //"#version 110 \n"
        "void main() \n"
        "{ \n"
          "gl_Position = ftransform(); \n"
          "gl_TexCoord[0] = gl_MultiTexCoord0; \n"
        "} \n"
        ;

char *persist_fs =
        //"#version 110 \n"
        "uniform sampler2D texel; \n"
        "void main() \n"
        "{ \n"
            // Get texel value
            "vec4 px = texture2D( texel, gl_TexCoord[0].xy ); \n"

            // Get luminance 0-1, all color channels have same value
            "float L = px.x; \n"

            // Determine colors
            "float R = clamp( (L-0.5)*4.0,       0.0, 1.0); \n"
            "float MG = clamp( 4.0*L - 3.0,      0.0, 1.0 ); \n"
            "float G =  clamp( 4.0*L,            0.0, 1.0 ) - MG; \n"
            "float B = clamp( (0.5-L)*4.0,       0.0, 1.0); \n"
            "float A =  px.w; \n"

            // Output frag color
            "gl_FragColor = vec4( R,G,B,A ); \n"
        "} \n"
        ;

//#include "../dialogs/numeric_entry.h"

// Functions act as intermediary to the numeric entry dialog
// If some aspect of the NumericEntry() class changes, the only place
//  to change the code will be here
//bool sa_lib::GetUserInput(const Frequency &fin, Frequency &fout)
//{
//    fout = NumericEntry::GetFrequency(fin);
//    return (fout != fin);
//}

//bool sa_lib::GetUserInput(const Amplitude &ain, Amplitude &aout)
//{
//    aout = NumericEntry::GetAmplitude(ain);
//    return (aout != ain);
//}

//bool sa_lib::GetUserInput(const Time &tin, Time &tout)
//{
//    tout = NumericEntry::GetTime(tin);
//    return (tin != tout);
//}

//bool sa_lib::GetUserInput(double din, double &dout)
//{
//    dout = NumericEntry::GetNumber(din);
//    return (din != dout);
//}

// Native bw look up table
bandwidth_lut native_bw_lut[] = {
    { 0.301003456, 536870912 },  //0
    { 0.602006912, 268435456 },  //1
    { 1.204013824, 134217728 },  //2
    { 2.408027649, 67108864 },   //3
    { 4.816055298, 33554432 },   //4
    { 9.632110596, 16777216 },   //5
    { 19.26422119, 8388608 },    //6
    { 38.52844238, 4194304 },    //7
    { 77.05688477, 2097152 },    //8
    { 154.1137695, 1048576 },    //9
    { 308.2275391, 524288 },     //10
    { 616.4550781, 262144 },     //11
    { 1232.910156, 131072 },     //12
    { 2465.820313, 65536 },      //13 rt streaming end
    { 4931.640625, 32768 },      //14
    { 9863.28125,  16384 },      //15
    { 19726.5625,  8192 },       //16
    { 39453.125,   4096 },       //17
    { 78906.25,    2048 },       //18
    { 157812.5,    1024 },       //19
    { 315625,      512 },        //20
    { 631250,      256 },        //21 rt streaming begin
    { 1262500,     128 },        //22
    { 2525000,     64 },         //23
    { 5050000,     32 },         //24
    { 10100000 ,   16 }          //25
};
const int native_bw_lut_sz =
        sizeof(native_bw_lut) / sizeof(bandwidth_lut);

span_to_bandwidth auto_bw_lut[] = {
    // span    native      non-native
    { 1.0e9,   315625,       3.0e5 },
    { 500.0e6, 157812.5,     1.0e5 },
    { 200.0e6, 78906.25,     1.0e5 },
    { 100.0e6, 39453.125,    3.0e4 },
    { 50.0e6,  19726.5625,   3.0e4 },
    { 20.0e6,  9863.28125,   1.0e4 },
    { 10.0e6,  4931.640625,  1.0e4 },
    { 5.0e6,   2465.820313,  3.0e3 },
    { 2.0e6,   1232.910156,  1.0e3 },
    { 1.0e6,   616.4550781,  1.0e3 },
    { 0.5e6,   308.2275391,  3.0e2 },
    { 0.2e6,   154.1137695,  1.0e2 }
};
const int auto_bw_lut_sz =
        sizeof(auto_bw_lut) / sizeof(span_to_bandwidth);

GLShader::GLShader(GLShaderType type, char *file_name)
    : compiled(GL_FALSE),
      shader_source(nullptr),
      shader_type(type),
      shader_handle(0)
{
    shader_source = file_name; //bb_lib::get_gl_shader_source(file_name);
}

GLShader::~GLShader()
{
    if(shader_source) {
        //delete [] shader_source;
    }
}

bool GLShader::Compile(QOpenGLFunctions *gl)
{
    if(compiled == GL_TRUE) {
        return true;
    }

    if(!shader_source) {
        return false;
    }

    if(shader_type == VERTEX_SHADER)
        shader_handle = gl->glCreateShader(GL_VERTEX_SHADER);
    else if(shader_type == FRAGMENT_SHADER)
        shader_handle = gl->glCreateShader(GL_FRAGMENT_SHADER);

    gl->glShaderSource(shader_handle, 1, (const char**)(&shader_source), 0);
    gl->glCompileShader(shader_handle);
    gl->glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &compiled);

    return Compiled();
}

GLProgram::GLProgram(char *vertex_file,
                     char *fragment_file)
    : vertex_shader(VERTEX_SHADER, vertex_file),
      fragment_shader(FRAGMENT_SHADER, fragment_file),
      compiled(GL_FALSE)
{

}

GLProgram::~GLProgram()
{

}

bool GLProgram::Compile(QOpenGLFunctions *gl)
{
    if(compiled == GL_TRUE) {
        return false;
    }

    program_handle = gl->glCreateProgram();

    vertex_shader.Compile(gl);
    fragment_shader.Compile(gl);

    gl->glAttachShader(program_handle, vertex_shader.Handle());
    gl->glAttachShader(program_handle, fragment_shader.Handle());

    //std::cout << "program handle : " << program_handle << std::endl;
    //std::cout << "vertex handle : " << vertex_shader.Handle() << std::endl;
    //std::cout << "fragment handle : " << fragment_shader.Handle() << std::endl;

    //int log_len;
    //char log[1000];
    //gl->glGetProgramInfoLog(program_handle, 999, &log_len, log);
    //gl->glGetShaderInfoLog(fragment_shader.Handle(), 999, &log_len, log);
    //std::cout << log << std::endl;


    // Link Program
    gl->glLinkProgram(program_handle);
    gl->glGetProgramiv(program_handle, GL_LINK_STATUS, &compiled);

    //gl->glGetProgramInfoLog(program_handle, 999, &log_len, log);
    //std::cout << log << std::endl;

    //std::cout << "Program Compiled " << compiled << std::endl;
    //if(compiled != GL_TRUE)
    //    exit(-1);

    return Compiled();
}

GLuint get_texture_from_file(const QString &file_name)
{
    QImage image(file_name);

    unsigned char *my_img = (unsigned char*)malloc(image.width() * image.height() * 3);
    unsigned char *img_cpy = image.bits();

    int ix = 0;
    for(int i = 0; i < image.width() * image.height() * 4; i += 4) {
        my_img[ix++] = img_cpy[i];
        my_img[ix++] = img_cpy[i+1];
        my_img[ix++] = img_cpy[i+2];
    }

    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width(), image.height(), 0,
        GL_RGB, GL_UNSIGNED_BYTE, (unsigned char*)my_img);

    free(my_img);

    return t;
}

int bb_lib::cpy_16u(const ushort *src, ushort *dst, int maxCopy)
{
    int loc = 0;
    while((src[loc] != '\0') && (loc < (maxCopy-1))) {
        dst[loc] = src[loc];
        loc++;
    }
    dst[loc] = '\0';
    return loc;
}

int bb_lib::get_native_bw_index(double bw)
{
    int ix = 0;
    double dif = fabs(bw - native_bw_lut[ix++].bw);
    double ref = fabs(bw - native_bw_lut[ix].bw);

    while(ref < dif) {
        dif = ref;
        ref = fabs(bw - native_bw_lut[++ix].bw);
        if(ix >= (native_bw_lut_sz - 1))
            break;
    }

    return ix - 1;
}

// Find the next bandwidth in sequence
// If native, use table
// If non-native, use 1/3/10 sequence
double bb_lib::sequence_bw(double bw, bool native_bw, bool increase)
{
    if(native_bw) {
        int ix = get_native_bw_index(bw);

        if(increase) {
            ix++;
        } else {
            ix--;
        }

        clamp(ix, 0, native_bw_lut_sz);
        bw = native_bw_lut[ix].bw;

    } else { // (!native_bw)
        int factor = 0;

        while(bw >= 10.0) {
            bw /= 10.0;
            factor++;
        }

        if(increase) {
            if(bw >= 3.0) bw = 10.0;
            else bw = 3.0;
        } else {
            if(bw <= 1.0) bw = 0.3;
            else if(bw <= 3.0) bw = 1.0;
            else bw = 3.0;
        }

        while(factor--) {
            bw *= 10.0;
        }
    }

    return bw;
}

double bb_lib::sequence_span(double span, bool increase)
{
    int factor = 0;

    while(span >= 10.0) {
        span /= 10.0;
        factor++;
    }

    if(increase) {
        if(span >= 5.0) {
            span = 10.0;
        } else if(span >= 2.0) {
            span = 5.0;
        } else {
            span = 2.0;
        }
    } else {
        if(span <= 1.0) {
            span = 0.5;
        } else if(span <= 2.0) {
            span = 1.0;
        } else if(span <= 5.0) {
            span = 2.0;
        } else {
            span = 5.0;
        }
    }

    while(factor--) {
        span *= 10.0;
    }

    return span;
}

bool bb_lib::adjust_rbw_on_span(Frequency &rbw, double span, bool native_rbw)
{
    bool adjusted = false;

    while((span / rbw) > 500000) {
        adjusted = true;
        rbw = sequence_bw(rbw, native_rbw, true);
    }

    while((span / rbw) < 1.0) {
        adjusted = true;
        rbw = sequence_bw(rbw, native_rbw, false);
    }

    return adjusted;
}

// Auto RBW based on span
double bb_lib::get_best_rbw(double span, bool native_rbw)
{
    int best_ix = 0;
    double best_diff = fabs(span - auto_bw_lut[0].span);

    for(int i = 1; i < auto_bw_lut_sz; i++) {
        double d = fabs(span - auto_bw_lut[i].span);
        if(d < best_diff) {
            best_diff = d;
            best_ix = i;
        }
    }

    if(native_rbw) {
        return auto_bw_lut[best_ix].nbw;
    }

    return auto_bw_lut[best_ix].nnbw;
}

// Get Users MyDocuments path, append application directory
QString bb_lib::get_my_documents_path()
{
    QString path = QStandardPaths::
            writableLocation(QStandardPaths::DocumentsLocation);
    assert(path.length() > 0);
    path += "/SignalHound/";
    QDir().mkdir(path);
    return path;
}

char* bb_lib::get_gl_shader_source(const char *file_name)
{
    FILE *f = NULL;
    fopen_s(&f, file_name, "rb");
    if(f == NULL) {
        return 0; // Unable to open file
    }
    fseek(f, 0L, SEEK_END);
    int len = ftell(f);
    rewind(f);
    char *source = new char[len+1];
    if(fread(source, 1, len, f) != size_t(len)) {
        return 0; // Unable to read in file
    }
    source[len] = '\0';
    fclose(f);
    return source;
}

// Normalize frequency domain trace
void normalize_trace(const Trace *t, GLVector &v, QPoint grat_size)
{
    v.clear();

    if(t->Length() <= 0) {
        return;
    }

    // Data needed to transform sweep into a screen trace
    int currentPix = 0;                                   // Step through all pixels
    double currentStep = 0.0;                              // Our curr step
    double step = (float)(grat_size.x()) / ( t->Length() - 1 );   // Step size
    double ref;                       // Value representing the top of graticule
    double botRef;                    // Value representing bottom of graticule
    double xScale = 1.0 / (float)grat_size.x();
    double yScale;
    double dBdiv = t->GetSettings()->Div();

    Amplitude refLevel = t->GetSettings()->RefLevel();

    if(refLevel.IsLogScale()) {
        ref = refLevel.ConvertToUnits(AmpUnits::DBM);
        botRef = ref - 10.0 * dBdiv;
        yScale = 1.0 / (10.0 * dBdiv);
    } else {
        ref = refLevel.Val();
        botRef = 0.0;
        yScale = (1.0 / ref);
    }

//    ref = t->GetSettings()->RefLevel().Val();
//    botRef = ref - 10.0 * dBdiv;
//    yScale = 1.0 / (10.0 * dBdiv);
//    xScale = 1.0 / (float)grat_size.x();

//    if(!t->GetSettings()->RefLevel().IsLogScale()) {
//        botRef = 0.0;
//        yScale = (1.0 / ref);
//    }

    v.reserve(grat_size.x() * 4);

    // Less samples than pixels, create quads max1,min1,max2,min2,max3,min3,..
    if((int)t->Length() < grat_size.x()) {

        for(int i = 0; i < (int)t->Length(); i++) {

            double max = t->Max()[i];
            double min = t->Min()[i];

            v.push_back(xScale * (currentStep));
            v.push_back(yScale * (max - botRef));
            v.push_back(xScale * (currentStep));
            v.push_back(yScale * (min - botRef));

            currentStep += step;
        }
        return;
    }

    // Keeps track of local max/mins
    float min = ref;
    float max = botRef;

    // More samples than pixels, Create pixel resolution, keeps track
    //   of min/max for each pixel, draws 1 pixel wide quads
    for(int i = 0; i < t->Length(); i++) {

        double minVal = t->Min()[i];
        double maxVal = t->Max()[i];

        if(maxVal > max) max = maxVal;
        if(minVal < min) min = minVal;

        currentStep += step;

        if(currentStep > currentPix) {

            v.push_back(xScale * currentPix);
            v.push_back(yScale * (min - botRef));
            v.push_back(xScale * currentPix);
            v.push_back(yScale * (max - botRef));

            min = ref;
            max = botRef;
            currentPix++;
        }
    }
}

void normalize_trace(const Trace *t, LineList &ll, QSize grat_size)
{
    ll.clear();

    if(t->Length() <= 0) {
        return;
    }

    // Data needed to transform sweep into a screen trace
    int currentPix = 0;                                   // Step through all pixels
    double currentStep = 0.0;                              // Our curr step
    double step = (float)(grat_size.width()) / ( t->Length() - 1 );   // Step size
    double ref;                       // Value representing the top of graticule
    double botRef;                    // Value representing bottom of graticule
    double xScale, yScale;
    double dBdiv = t->GetSettings()->Div();

    // Non-FM modulation
    ref = t->GetSettings()->RefLevel().Val();
    botRef = ref - 10.0 * dBdiv;
    yScale = 1.0 / (10.0 * dBdiv);

    xScale = 1.0 / (float)grat_size.width();

    ll.reserve(grat_size.width() * 4);

    // Less samples than pixels, create quads max1,min1,max2,min2,max3,min3,..
    if((int)t->Length() < grat_size.width()) {

        for(int i = 0; i < (int)t->Length(); i++) {

            double max = t->Max()[i];
            double min = t->Min()[i];

            ll.push_back(QPointF(xScale * (currentStep),
                                 yScale * (max - botRef)));
            ll.push_back(QPointF(xScale * (currentStep),
                                 yScale * (min - botRef)));

            currentStep += step;
        }
        return;
    }

    // Keeps track of local max/mins
    float min = ref;
    float max = botRef;

    // More samples than pixels, Create pixel resolution, keeps track
    //   of min/max for each pixel, draws 1 pixel wide quads
    for(int i = 0; i < t->Length(); i++) {

        double minVal = t->Min()[i];
        double maxVal = t->Max()[i];

        if(maxVal > max) max = maxVal;
        if(minVal < min) min = minVal;

        currentStep += step;

        if(currentStep > currentPix) {

            ll.push_back(QPointF(xScale * currentPix,
                                 yScale * (min - botRef)));
            ll.push_back(QPointF(xScale * currentPix,
                                 yScale * (max - botRef)));

            min = ref;
            max = botRef;
            currentPix++;
        }
    }
}