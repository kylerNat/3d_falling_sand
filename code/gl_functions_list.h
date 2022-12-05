#ifdef gl_check
#define gl_start_check {
#define gl_end_check }
#else
#define gl_check(extension)
#define gl_start_check
#define gl_end_check
#endif

gl_load_operation(void, glDebugMessageCallbackARB, (GLDEBUGPROCARB callback, const void* userParam));

gl_load_operation(void, glMemoryBarrier, (GLbitfield barriers));

gl_load_operation(void, glGenBuffers, (GLsizei n, const GLuint * buffers));
gl_load_operation(void, glDrawBuffers, (GLsizei n, GLenum * buffers));
gl_load_operation(void, glBufferStorage, (GLenum target, GLsizei ptrsize, const GLvoid * data, GLbitfield flags));
gl_load_operation(void, glBufferData, (GLenum target, GLsizei ptrsize, const GLvoid * data, GLenum usage));
gl_load_operation(void, glBufferSubData, (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data));
gl_load_operation(void, glGetBufferSubData, (GLenum target, GLintptr offset, GLsizeiptr size, GLvoid * data));
gl_load_operation(void, glClearBufferSubData, (GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void * data));
gl_load_operation(void, glBindBuffer, (GLenum target, GLuint buffer));
gl_load_operation(void, glBindBufferBase, (GLenum target, GLuint index, GLuint buffer));
gl_load_operation(void, glBindBufferRange, (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size));
gl_load_operation(void, glVertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer));
gl_load_operation(void, glEnableVertexAttribArray, (GLuint index));
gl_load_operation(void, glVertexAttribDivisor, (GLuint index, GLuint divisor));
gl_load_operation(void, glDrawArraysInstanced, (GLenum mode, GLint first, GLsizei count, GLsizei primcount));
gl_load_operation(void, glDrawElementsInstanced, (GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount));

gl_load_operation(void, glBindImageTexture, (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format));

gl_load_operation(void, glGetBufferParameteriv, (GLenum target, GLenum value, GLint * data));

gl_load_operation(void, glGenVertexArrays, (GLsizei n, GLuint *arrays));
gl_load_operation(void, glBindVertexArray, (GLuint array));

gl_load_operation(GLuint, glCreateShader, (GLenum shaderType));
gl_load_operation(void, glShaderSource, (GLuint shader, GLsizei count, const GLchar * const *string, const GLint *length));
gl_load_operation(void, glCompileShader, (GLuint shader));
gl_load_operation(void, glGetShaderiv, (GLuint shader, GLenum pname, GLint *params));
gl_load_operation(void, glGetShaderInfoLog, (GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog));
gl_load_operation(GLuint, glCreateProgram, (void));
gl_load_operation(void, glAttachShader, (GLuint program, GLuint shader));
gl_load_operation(void, glLinkProgram, (GLuint program));
gl_load_operation(void, glGetProgramiv, (GLuint program, GLenum pname, GLint *params));
gl_load_operation(void, glGetProgramInfoLog, (GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog));
gl_load_operation(void, glDetachShader, (GLuint program, GLuint shader));
gl_load_operation(void, glDeleteShader, (GLuint shader));
gl_load_operation(void, glUseProgram, (GLuint program));

gl_load_operation(void, glActiveTexture, (GLenum texture));
gl_load_operation(void, glGenerateMipmap, (GLenum target));

gl_load_operation(GLuint, glGetUniformLocation, (GLuint program, const GLchar * name));
gl_load_operation(void, glUniform1i, (GLuint location, GLint v0));
gl_load_operation(void, glUniform2i, (GLuint location, GLint v0, GLint v1));
gl_load_operation(void, glUniform1f, (GLuint location, GLfloat v0));
gl_load_operation(void, glUniform2f, (GLuint location, GLfloat v0, GLfloat v1));
gl_load_operation(void, glUniform3f, (GLuint location, GLfloat v0, GLfloat v1, GLfloat v2));
gl_load_operation(void, glUniform2fv, (GLuint location, GLsizei count, GLfloat* value));
gl_load_operation(void, glUniform3iv, (GLuint location, GLsizei count, GLint* value));
gl_load_operation(void, glUniform3fv, (GLuint location, GLsizei count, GLfloat* value));
gl_load_operation(void, glUniform4fv, (GLuint location, GLsizei count, GLfloat* value));
gl_load_operation(void, glUniform4iv, (GLuint location, GLsizei count, GLint* value));
gl_load_operation(void, glUniformMatrix3fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value));
gl_load_operation(void, glUniformMatrix4fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value));

gl_load_operation(void, glVertexAttrib2f, (GLuint index, GLfloat v0, GLfloat v1));
gl_load_operation(void, glVertexAttrib3f, (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2));
gl_load_operation(void, glVertexAttrib4f, (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3));

gl_load_operation(void, glPatchParameteri, (GLenum pname, GLint value));

gl_load_operation(void, glGenRenderbuffers, (GLsizei n, GLuint *renderbuffers));
gl_load_operation(void, glBindRenderbuffer, (GLenum target, GLuint renderbuffer));
gl_load_operation(void, glRenderbufferStorage, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height));
gl_load_operation(void, glGenFramebuffers, (GLsizei n, GLuint *framebuffers));
gl_load_operation(void, glBindFramebuffer, (GLenum target, GLuint framebuffer));
gl_load_operation(void, glFramebufferTexture, (GLenum target, GLenum attachment, GLuint texture, GLint level));
gl_load_operation(void, glFramebufferTexture2D, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level));
gl_load_operation(void, glFramebufferTextureLayer, (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer));
gl_load_operation(void, glFramebufferRenderbuffer, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer));
gl_load_operation(GLenum, glCheckFramebufferStatus, (GLenum target));
gl_load_operation(void, glTexImage2DMultisample, (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations));
gl_load_operation(void, glTexImage3D, (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void * data));
gl_load_operation(void, glClearTexImage, (GLuint texture, GLint level, GLenum format, GLenum type, const void * data));
gl_load_operation(void, glClearTexSubImage, (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void * data));
gl_load_operation(void, glClampColor, (GLuint target, GLenum clamp));
gl_load_operation(void, glRenderbufferStorageMultisample, (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height));
gl_load_operation(void, glCopyImageSubData, (GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth));

gl_load_operation(void, glTextureSubImage2D, (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels));
gl_load_operation(void, glTexSubImage3D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels));
gl_load_operation(void, glTexStorage1D, (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width));
gl_load_operation(void, glTexStorage2D, (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height));
gl_load_operation(void, glTexStorage3D, (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth));
gl_load_operation(void, glGetTextureSubImage, (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels));
gl_load_operation(void, glBlitFramebuffer, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter));

gl_load_operation(void, glDispatchCompute, (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z));

gl_check("WGL_EXT_swap_control")
gl_start_check
gl_load_operation(BOOL, wglSwapIntervalEXT, (int interval));
gl_load_operation(int, wglGetSwapIntervalEXT, (void));
gl_end_check

gl_load_operation(HGLRC, wglCreateContextAttribsARB, (HDC hDC, HGLRC hShareContext, const int *attribList));

gl_load_operation(void, wglChoosePixelFormatARB, (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats));

#undef gl_load_operation
#undef gl_check
#undef gl_start_check
#undef gl_end_check
#undef declaring
