#include "bsg.h"

namespace bsg {

void bsgUtils::printMat(const std::string& name, const glm::mat4& mat) {

  std::cout << name << std::endl;  
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      printf("%8.4f ", mat[j][i]);
    }
    std::cout << std::endl;
  }
}
  
// Get a handle for our lighting uniforms.  We are not binding the
// attribute to a known location, just asking politely for it.  Note
// that what is going on here is that OpenGL is actually matching
// the _lightPositionName string to a variable in the shader.
//
// This must be preceded by a glUseProgram(programID) call.
void lightList::load(const GLint programID) {

  // If there aren't any lights, don't bother.
  if (_lightPositions.size() > 0) {
  
    _lightPositions.ID = glGetUniformLocation(programID,
                                              _lightPositions.name.c_str());
    _lightColors.ID = glGetUniformLocation(programID,
                                           _lightColors.name.c_str());
  }
}

// Update any changes to the light's position and color.  This must be
// preceded by a glUseProgram(programID) call.
void lightList::draw() {

  // If there aren't any lights, don't bother.
  if (_lightPositions.size() > 0) {
    glUniform4fv(_lightPositions.ID,
                 _lightPositions.getData().size(),
                 &_lightPositions.getData()[0].x);
    glUniform4fv(_lightColors.ID,
                 _lightColors.getData().size(),
                 &_lightColors.getData()[0].x);
  }
}

void textureMgr::readFile(const textureType& type, const std::string& fileName) {

  switch(type) {
  case textureDDS:
    throw std::runtime_error("still working on DDS, try PNG");
    break;

  case textureBMP:
    throw std::runtime_error("still working on BMP, try PNG");
    break;

  case texturePNG:
    _textureBufferID = _loadPNG(fileName);
    break;

  case textureCHK:
    _textureBufferID = _loadCheckerBoard (64, 8);
    break;
    
  default:
    throw std::runtime_error("What texture type is this?");
  }
}

GLuint textureMgr::_loadCheckerBoard (int size, int numFields) {
  _width = size;
  _height = size;
  int fieldWidth = size/numFields;

  GLubyte image[size][size][3];

  // Create a checkerboard pattern
  for ( int i = 0; i < size; i++ ) {
    for ( int j = 0; j < size; j++ ) {
      GLubyte c = 31;
      if ((i/fieldWidth)%2 == (j/fieldWidth)%2) {c = 255;}
        image[i][j][0]  = c;
        image[i][j][1]  = c;
        image[i][j][2]  = c;
      }
    }

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width, _height,
               0, GL_RGB, GL_UNSIGNED_BYTE, image);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  return texture;
}


GLuint textureMgr::_loadPNG(const std::string imagePath) {
  
  // This function was originally written by David Grayson for
  // https://github.com/DavidEGrayson/ahrs-visualizer

  png_byte header[8];

  FILE *fp = fopen(imagePath.c_str(), "rb");
  if (fp == 0) {
    perror(imagePath.c_str());
    return 0;
  }

  // read the header
  fread(header, 1, 8, fp);

  if (png_sig_cmp(header, 0, 8)) {
    fprintf(stderr, "error: %s is not a PNG.\n", imagePath.c_str());
    fclose(fp);
    return 0;
  }

  png_structp png_ptr =
    png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fprintf(stderr, "error: png_create_read_struct returned 0.\n");
    fclose(fp);
    return 0;
  }

  // create png info struct
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    fprintf(stderr, "error: png_create_info_struct returned 0.\n");
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    fclose(fp);
    return 0;
  }

  // create png info struct
  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info) {
    fprintf(stderr, "error: png_create_info_struct returned 0.\n");
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
    fclose(fp);
    return 0;
  }

  // the code in this if statement gets called if libpng encounters an error
  // if (setjmp(png_jmpbuf(png_ptr))) {
  //   fprintf(stderr, "error from libpng\n");
  //   png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  //   fclose(fp);
  //   return 0;
  // }

  // init png reading
  png_init_io(png_ptr, fp);

  // let libpng know you already read the first 8 bytes
  png_set_sig_bytes(png_ptr, 8);

  // read all the info up to the image data
  png_read_info(png_ptr, info_ptr);

  // variables to pass to get info
  int bit_depth, color_type;
  png_uint_32 temp_width, temp_height;

  // get info about png
  png_get_IHDR(png_ptr, info_ptr, &temp_width, &temp_height,
               &bit_depth, &color_type,
               NULL, NULL, NULL);

  _width = temp_width;
  _height = temp_height;
  
  //printf("%s: %lux%lu %d\n", imagePath, temp_width, temp_height, color_type);

  if (bit_depth != 8) {
    fprintf(stderr, "%s: Unsupported bit depth %d.  Must be 8.\n", imagePath.c_str(), bit_depth);
    return 0;
  }

  GLint format;
  switch(color_type) {
  case PNG_COLOR_TYPE_RGB:
    format = GL_RGB;
    break;
  case PNG_COLOR_TYPE_RGB_ALPHA:
    format = GL_RGBA;
    break;
  default:
    fprintf(stderr, "%s: Unknown libpng color type %d.\n", imagePath.c_str(), color_type);
    return 0;
  }

  // Update the png info struct.
  png_read_update_info(png_ptr, info_ptr);

  // Row size in bytes.
  int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

  // glTexImage2d requires rows to be 4-byte aligned
  rowbytes += 3 - ((rowbytes-1) % 4);

  // Allocate the image_data as a big block, to be given to opengl
  png_byte * image_data = (png_byte *)malloc(rowbytes * temp_height * sizeof(png_byte)+15);
  if (image_data == NULL) {
    fprintf(stderr, "error: could not allocate memory for PNG image data\n");
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);
    return 0;
  }

  // row_pointers is for pointing to image_data for reading the png with libpng
  png_byte ** row_pointers = (png_byte **)malloc(temp_height * sizeof(png_byte *));
  if (row_pointers == NULL) {
    fprintf(stderr, "error: could not allocate memory for PNG row pointers\n");
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    free(image_data);
    fclose(fp);
    return 0;
  }

  // set the individual row_pointers to point at the correct offsets of image_data
  for (unsigned int i = 0; i < temp_height; i++) {
    row_pointers[temp_height - 1 - i] = image_data + i * rowbytes;
  }

  // read the png into image_data through row_pointers
  png_read_image(png_ptr, row_pointers);

  // Generate the OpenGL texture object
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, format, temp_width, temp_height,
               0, format, GL_UNSIGNED_BYTE, image_data);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // clean up
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  free(image_data);
  free(row_pointers);
  fclose(fp);
  return texture;
}

void textureMgr::load(const GLuint programID) {

  // Get a handle for the texture uniform.
  _textureAttribID = glGetUniformLocation(programID,
                                          _textureAttribName.c_str());
}

void textureMgr::draw() {

  // Bind the texture in Texture Unit 0
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _textureBufferID);
  
  // Set our "myTextureSampler" sampler to user Texture Unit 0
  glUniform1i(_textureAttribID, 0);

  // The data is actually loaded into the buffer in the loadXX() method.
}
  
std::string shaderMgr::_getShaderInfoLog(GLuint obj) {
  int infoLogLength = 0;
  int charsWritten  = 0;
  char *infoLog;
  GLint result = GL_FALSE;
  glGetShaderiv(obj, GL_COMPILE_STATUS, &result);
  glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infoLogLength);
 
  if (infoLogLength > 1) {
    infoLog = (char *)malloc(infoLogLength);
    glGetShaderInfoLog(obj, infoLogLength, &charsWritten, infoLog);
    return std::string(infoLog);
  } else {
    return std::string("");
  }
}

std::string shaderMgr::_getProgramInfoLog(GLuint obj) {
  
  int infologLength = 0;
  int charsWritten  = 0;
  char *infoLog;
  glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
 
  if (infologLength > 1) {
    infoLog = (char *)malloc(infologLength);
    glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
    return std::string(infoLog);
  } else {
    return std::string("");
  }
}
  
void shaderMgr::addShader(const GLSHADERTYPE type,
                          const std::string& shaderFile) {

  // Read the shader code from the given file.
  std::ifstream shaderStream(shaderFile.c_str(), std::ios::in);
  if (shaderStream.is_open()) {
    std::string line = "";

    // Suck all the text into the appropriate _shaderText slot.
    while(getline(shaderStream, line)) _shaderText[type] += "\n" + line;

    // Close the stream.
    shaderStream.close();
    
  } else {
    throw std::runtime_error("Cannot open: " + shaderFile);
  }

  _shaderFiles[type] = shaderFile;

  if (_lightList->getNumLights() > 0) {
    // Edit the shader source to reflect the input number of lights.  If
    // there is no 'XX' in the shader code, this will cause an ugly
    // error we have to catch.
    char numLightsAsString[5];
    sprintf(numLightsAsString, "%d", _lightList->getNumLights());
    try {
      _shaderText[type].replace(_shaderText[type].find("XX"), 2,
                                numLightsAsString);
    } catch (...) {
      std::cerr << "Caution: Shader ("
                << shaderFile
                << ") does not care about number of lights." << std::endl;
    }
  }
}

void shaderMgr::compileShaders() {

  // geom is true if there *is* a geometry shader in place.
  bool geom = (!_shaderText[GLSHADER_GEOMETRY].empty());

  _shaderIDs[GLSHADER_VERTEX] = glCreateShader(GL_VERTEX_SHADER);
  _shaderIDs[GLSHADER_FRAGMENT] = glCreateShader(GL_FRAGMENT_SHADER);
  if (geom) _shaderIDs[GLSHADER_GEOMETRY] = glCreateShader(GL_GEOMETRY_SHADER);

  // The OpenGL calls don't really like the modern C++ types, so we
  // convert back to old-fashioned char*.
  const char* vs = _shaderText[GLSHADER_VERTEX].c_str();
  const char* fs = _shaderText[GLSHADER_FRAGMENT].c_str();
  const char* gs;
  if (geom) gs = _shaderText[GLSHADER_GEOMETRY].c_str();

  // Feed the shader source to OpenGL.
  glShaderSource(_shaderIDs[GLSHADER_VERTEX], 1, &vs, NULL);
  glShaderSource(_shaderIDs[GLSHADER_FRAGMENT], 1, &fs, NULL);
  if (geom) glShaderSource(_shaderIDs[GLSHADER_GEOMETRY], 1, &gs, NULL);

  // Compile the shader source.  If there are any errors, print them.
  std::string errorLog;
  glCompileShader(_shaderIDs[GLSHADER_VERTEX]);
  errorLog = _getShaderInfoLog(_shaderIDs[GLSHADER_VERTEX]);
  if (errorLog.size() > 1) {
    std::cerr << "** Vertex compile error in "
              << _shaderFiles[GLSHADER_VERTEX]
              << std::endl << errorLog << std::endl;
    //std::cerr << _shaderText[GLSHADER_VERTEX] << std::endl;
  }
  
  
  glCompileShader(_shaderIDs[GLSHADER_FRAGMENT]);
  errorLog = _getShaderInfoLog(_shaderIDs[GLSHADER_FRAGMENT]);
  if (errorLog.size() > 1)
    std::cerr << "** Fragment compile error in "
              << _shaderFiles[GLSHADER_FRAGMENT]
              << std::endl << errorLog << std::endl;

  if (geom) {
    glCompileShader(_shaderIDs[GLSHADER_GEOMETRY]);
    errorLog = _getShaderInfoLog(_shaderIDs[GLSHADER_GEOMETRY]);
    if (errorLog.size() > 1)
      std::cerr << "** Geometry compile error in "
                << _shaderFiles[GLSHADER_GEOMETRY]
                << std::endl << errorLog << std::endl;
  }

  // Now create a program to contain our two (or three) shaders, and
  // attach the shaders to it.
  _programID = glCreateProgram();
  glAttachShader(_programID, _shaderIDs[GLSHADER_VERTEX]);
  glAttachShader(_programID, _shaderIDs[GLSHADER_FRAGMENT]);
  if (geom) glAttachShader(_programID, _shaderIDs[GLSHADER_GEOMETRY]);

  // Assemble the shaders into a single program with 'link', which
  // will make sure that the inputs to the fragment shader correspond
  // with outputs from the vertex shader, and so on.
  glLinkProgram(_programID);
  errorLog = _getProgramInfoLog(_programID);
  if (errorLog.size() > 1) {
    std::cerr << "** Shader link error in"
              << _shaderFiles[GLSHADER_VERTEX] << ", "
              << _shaderFiles[GLSHADER_FRAGMENT] << ", "
              << _shaderFiles[GLSHADER_GEOMETRY]
              << std::endl << errorLog << std::endl;
  } 

  // The shaders are linked into the program, so we can delete the raw
  // shaders.
  glDeleteShader(_shaderIDs[GLSHADER_VERTEX]);
  glDeleteShader(_shaderIDs[GLSHADER_FRAGMENT]);
  if (geom) glDeleteShader(_shaderIDs[GLSHADER_GEOMETRY]);

  _compiled = true;
}

GLuint shaderMgr::getAttribID(const std::string& attribName) {
  glUseProgram(_programID);
  return glGetAttribLocation(_programID, attribName.c_str());
}
 
GLuint shaderMgr::getUniformID(const std::string& unifName) {
  glUseProgram(_programID);
  return glGetUniformLocation(_programID, unifName.c_str());
}

void shaderMgr::addLights(const bsgPtr<lightList> lightList) {
  if (_compiled) {
    throw std::runtime_error("Must load lights before compiling shader.");
  } else {
    _lightList = lightList;
  }
}

void shaderMgr::load() {
  _lightList->load(_programID);
  if (_textureLoaded) _texture->load(_programID);
}

void shaderMgr::draw() {
  _lightList->draw();
  if (_textureLoaded) _texture->draw();
}

void drawableObj::addData(const GLDATATYPE type,
                          const std::string& name,
                          const std::vector<glm::vec4>& data) {

  switch(type) {
  case(GLDATA_VERTICES):
    _vertices = drawableObjData<glm::vec4>(name, data);
    break;
  case(GLDATA_COLORS):
    _colors = drawableObjData<glm::vec4>(name, data);
    break;
  case(GLDATA_NORMALS):
    _normals = drawableObjData<glm::vec4>(name, data);
    break;
  case(GLDATA_TEXCOORDS):
    throw std::runtime_error("Do not use vec4 for texture coordinates.");
    break;
  }
}

void drawableObj::addData(const GLDATATYPE type,
             const std::string& name,
             const std::vector<glm::vec2>& data) {

  switch(type) {
  case(GLDATA_TEXCOORDS):
    _uvs = drawableObjData<glm::vec2>(name, data);
    break;
  case(GLDATA_COLORS):
  case(GLDATA_NORMALS):
  case(GLDATA_VERTICES):
    throw std::runtime_error("Vec2 is only for texture coordinates.");
    break;
  }
}


  
void drawableObj::prepare(GLuint programID) {

  bool badID = false;
  
  // Figure out which buffers we need and get IDs for them.
  glGenBuffers(1, &_vertices.bufferID);  
  _vertices.ID = glGetAttribLocation(programID, _vertices.name.c_str());

  // Check to make sure the ID awarded is sane.  If not, probably the
  // name does not match the name in the shader.
  if (_vertices.ID < 0) {
    std::cerr << "** Caution: Bad ID for vertices attribute '" << _vertices.name << "'" << std::endl;
    badID = true;
  }
  
  if (!_colors.getData().empty()) {
    glGenBuffers(1, &_colors.bufferID);
    _colors.ID = glGetAttribLocation(programID, _colors.name.c_str());
    
    if (_colors.ID < 0) {
      std::cerr << "** Caution: Bad ID for colors attribute '" << _colors.name << "'" << std::endl;
      badID = true;
    }
  }
  if (!_normals.getData().empty()) {
    glGenBuffers(1, &_normals.bufferID);
    _normals.ID = glGetAttribLocation(programID, _normals.name.c_str());
    
    if (_normals.ID < 0) {
      std::cerr << "** Caution: Bad ID for normals attribute '" << _normals.name << "'" << std::endl;
      badID = true;
    }
  }
  if (!_uvs.getData().empty()) {
    glGenBuffers(1, &_uvs.bufferID);
    _uvs.ID = glGetAttribLocation(programID, _uvs.name.c_str());
    
    if (_uvs.ID < 0) {
      std::cerr << "** Caution: Bad ID for texture attribute '" << _uvs.name << "'" << std::endl;
      badID = true;
    }
  }

  if (badID) {
    std::cerr << "This can be caused either by a spelling error, or by not using the" << std::endl << "attribute within the shader code." << std::endl;
  }
  
  // Put the data in its buffers, for practice.
  load();

}

void drawableObj::load() {

  glBindBuffer(GL_ARRAY_BUFFER, _vertices.bufferID);
  glBufferData(GL_ARRAY_BUFFER, _vertices.size(), &_vertices.getData()[0], GL_STATIC_DRAW);

  if (!_colors.getData().empty()) {
    glBindBuffer(GL_ARRAY_BUFFER, _colors.bufferID);
    glBufferData(GL_ARRAY_BUFFER, _colors.size(), &_colors.getData()[0], GL_STATIC_DRAW);
  }
  if (!_normals.getData().empty()) {
    glBindBuffer(GL_ARRAY_BUFFER, _normals.bufferID);
    glBufferData(GL_ARRAY_BUFFER, _normals.size(), &_normals.getData()[0], GL_STATIC_DRAW);
  }
  if (!_uvs.getData().empty()) {
    glBindBuffer(GL_ARRAY_BUFFER, _uvs.bufferID);
    glBufferData(GL_ARRAY_BUFFER, _uvs.size(), &_uvs.getData()[0], GL_STATIC_DRAW);
  }
}

void drawableObj::draw() {

  glBindBuffer(GL_ARRAY_BUFFER, _vertices.bufferID);
  glEnableVertexAttribArray(_vertices.ID);
  glVertexAttribPointer(_vertices.ID, _vertices.intSize(), GL_FLOAT, 0, 0, 0);

  if (!_colors.getData().empty()) {
    glBindBuffer(GL_ARRAY_BUFFER, _colors.bufferID);
    glEnableVertexAttribArray(_colors.ID);
    glVertexAttribPointer(_colors.ID, _colors.intSize(), GL_FLOAT, 0, 0, 0);
  }
  if (!_normals.getData().empty()) {
    glBindBuffer(GL_ARRAY_BUFFER, _normals.bufferID);
    glEnableVertexAttribArray(_normals.ID);
    glVertexAttribPointer(_normals.ID, _normals.intSize(), GL_FLOAT, 0, 0, 0);
  }
  if (!_uvs.getData().empty()) {
    glBindBuffer(GL_ARRAY_BUFFER, _uvs.bufferID);
    glEnableVertexAttribArray(_uvs.ID);
    glVertexAttribPointer(_uvs.ID, _uvs.intSize(), GL_FLOAT, 0, 0, 0);
  }

  glDrawArrays(_drawType, 0, _count);
}

glm::mat4 drawableMulti::getModelMatrix() {

  if (_modelMatrixNeedsReset) {
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), _position);
    glm::mat4 rotationMatrix = glm::mat4_cast(_orientation);
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), _scale);

    _modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
    _modelMatrixNeedsReset = false;

    //    std::cout << glm::to_string(_modelMatrix) << std::endl;
    
    // bsgUtils::printMat("trans:", translationMatrix);
    // bsgUtils::printMat("rotat:", rotationMatrix);
    // bsgUtils::printMat("scale:", scaleMatrix);
    // bsgUtils::printMat("model:", _modelMatrix);
  }

  // If there is a parent, get the parent transformation (model)
  // matrix and use it with this one.
  if (_parent) 
    return _parent->getModelMatrix() * _modelMatrix;
  else
    return _modelMatrix;
}

void drawableCompound::prepare() {

  _pShader->useProgram();
  _pShader->prepare();

  _modelMatrixID = _pShader->getUniformID(_modelMatrixName);
  _normalMatrixID = _pShader->getUniformID(_normalMatrixName);
  _viewMatrixID = _pShader->getUniformID(_viewMatrixName);
  _projMatrixID = _pShader->getUniformID(_projMatrixName);

  // Prepare each component object.
  for (std::list<drawableObj>::iterator it = _objects.begin();
       it != _objects.end(); it++) {
    it->prepare(_pShader->getProgram());
  }
}
  
void drawableCompound::load() {

  _pShader->useProgram();
  _pShader->load();

  // Review the current state of the transformation matrices, and pack
  // them all into the total model matrix.
  _totalModelMatrix = getModelMatrix();
  
  // Load each component object.
  for (std::list<drawableObj>::iterator it = _objects.begin();
       it != _objects.end(); it++) {
    it->load();
  }
}

void drawableCompound::draw(const glm::mat4& viewMatrix,
                            const glm::mat4& projMatrix) {

  _pShader->useProgram();
  _pShader->draw();
  
  // Load the model matrix.  This adjusts the position of each object.
  // Remember that all the objects in a compound object use the same
  // shader and the same model matrix.
  glUniformMatrix4fv(_modelMatrixID, 1, false, &_totalModelMatrix[0][0]);

  // Calculate the normal matrix to use for lighting.
  _normalMatrix = glm::transpose(glm::inverse(viewMatrix * _totalModelMatrix));
  glUniformMatrix4fv(_normalMatrixID, 1, false, &_normalMatrix[0][0]);

  // The view and projection matrices come from the scene object, above us.
  glUniformMatrix4fv(_viewMatrixID, 1, false, &viewMatrix[0][0]);
  glUniformMatrix4fv(_projMatrixID, 1, false, &projMatrix[0][0]);

  // std::cout << "view" << glm::to_string(viewMatrix) << std::endl;
  // std::cout << "normal" << glm::to_string(_normalMatrix) << std::endl;
  // std::cout << "model" << glm::to_string(_modelMatrix) << std::endl;
  // std::cout << "proj" << glm::to_string(projMatrix) << std::endl;
  
  for (std::list<drawableObj>::iterator it = _objects.begin();
       it != _objects.end(); it++) {
    it->draw();
  }  
}

drawableCollection::drawableCollection() {
  // Seed a random number generator to generate default names randomly.
  struct timeval tp;
  gettimeofday(&tp, NULL);
  srand(tp.tv_usec);
}

drawableCollection::drawableCollection(const std::string name) :
  drawableMulti(name) {
  // Seed a random number generator to generate default names randomly.
  struct timeval tp;
  gettimeofday(&tp, NULL);
  srand(tp.tv_usec);
}

std::string drawableCollection::addObject(const std::string name,
                                   const bsgPtr<drawableMulti> &pMultiObject) {
  pMultiObject->setParent(this);
  _collection[name] = pMultiObject;

  return name;
}

std::string drawableCollection::addObject(const bsgPtr<drawableMulti> &pMultiObject) {

  if (pMultiObject->getName().empty()) {

    return addObject(randomName(), pMultiObject);

  } else {
    if (_collection.find(pMultiObject->getName()) != _collection.end()) {

      std::cerr << "You have already used " << pMultiObject->getName() << " in " << getName() << ".  Assigning a random name." << std::endl;

      return addObject(randomName(), pMultiObject);

    } else {
      
      return addObject(pMultiObject->getName(), pMultiObject);
    }
  }
}

bsgPtr<drawableMulti> drawableCollection::getObject(const std::string name) {

  CollectionMap::iterator it = _collection.find(name);

  // Throwing an error might be a little harsh.
  if (it == _collection.end()) {
    throw std::runtime_error("what object is " + name + "?");
  } else {
    return it->second;
  }
}

std::list<std::string> drawableCollection::getNames() {
  
  std::list<std::string> out;
  for (CollectionMap::iterator it = _collection.begin();
       it != _collection.end(); it++) {
    out.push_back(it->first);
  }

  return out;
}    

std::string drawableCollection::randomName() {

  // This is a pretty dopey method, but it seems to work, so long as
  // the number of characters in each name is big enough.
  std::string out = "";
  for(int i = 0; i < 6; i++) {
    switch(rand()%3) {
    case 0:
      out += ('0' + rand()%10);
      break;
    case 1:
      out += ('A' + rand()%26);
      break;
    case 2:
      out += ('a' + rand()%26);
      break; 
    }
  }
  return out;
}



  

  
void drawableCollection::prepare() {

  for (CollectionMap::iterator it =  _collection.begin();
       it != _collection.end(); it++) {
    it->second->prepare();
  }
}

void drawableCollection::load() {

  // Then draw all the objects.
  for (CollectionMap::iterator it =  _collection.begin();
       it != _collection.end(); it++) {
    it->second->load();
  }
}

void drawableCollection::draw(const glm::mat4 &viewMatrix,
                              const glm::mat4 &projMatrix) {

  // Then draw all the objects.
  for (CollectionMap::iterator it =  _collection.begin();
       it != _collection.end(); it++) {
    it->second->draw(viewMatrix, projMatrix);
  }
}


/// \brief Adjust camera position according to input Euler angles.
///
/// We use quaternions in the implementation because they provide a
/// smooth way to rotate continuously.  You'll notice an oddness in
/// the rotation in practice, because the up vector is always the same
/// direction.
void scene::addToCameraViewAngle(const float horizAngle, const float vertAngle) {

  // Calculate current direction vector.
  glm::vec3 dir = _lookAtPosition - _cameraPosition;

  // Calculate a rotation quaternion based on the input euler angles.
  glm::quat rot = glm::quat(glm::vec3(vertAngle, horizAngle, 0.0f));

  // Apply the quaternion to the direction vector.
  glm::vec4 newDir = glm::rotate(rot, glm::vec4(dir.x, dir.y, dir.z, 1.0f));

  // Apply the direction vector to the lookat position to get the new
  // camera location.
  _cameraPosition = _lookAtPosition - glm::vec3(newDir.x, newDir.y, newDir.z);
}
  
void scene::prepare() {

  _sceneRoot.prepare();
}

glm::mat4 scene::getProjMatrix() {
  // Update the projection matrix.  In case of a stereo display, both
  // eyes will use the same projection matrix.
  return glm::perspective(_fov, _aspect, _nearClip, _farClip);
}

glm::mat4 scene::getViewMatrix() {
  // Update the view matrix.
  glm::vec3 dir = glm::normalize(_lookAtPosition - _cameraPosition);
  glm::vec3 right = glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f));
  glm::vec3 up = glm::normalize(glm::cross(right, dir));

  return glm::lookAt(_cameraPosition, _lookAtPosition, up);
}   
  
void scene::load() {

  _sceneRoot.load();
}

void scene::draw(const glm::mat4 &viewMatrix,
                 const glm::mat4 &projMatrix) {

  _sceneRoot.draw(viewMatrix, projMatrix);
}  
  
}
