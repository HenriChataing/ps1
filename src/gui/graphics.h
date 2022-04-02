
#include <GL/glew.h>
#include <GLFW/glfw3.h>

/** Refresh the screen, called once during vertical blank. */
void refreshVideoImage(void);

/** Return the ID of a texture copied from the current video image, or
 * 0 if no vido image is set. */
bool getVideoImage(size_t *width, size_t *height, GLuint *id);

/** Export the current video image frame buffer in PNG format. */
void exportAsPNG(char const *filename);
