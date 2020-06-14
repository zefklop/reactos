/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         Tests software implementation of glTexImage2D
 * PROGRAMMERS:     Jérôme Gardou
 */

#include <windows.h>
#include <wingdi.h>
#include <GL/gl.h>

#include "wine/test.h"

#define ok_color_channel(x, y) ok(abs(x - y) <= 1, "Wrong value for " #x ", got 0x%02x expected " #y " (0x%02x)\n", x, y)

START_TEST(sw_teximage)
{
    BITMAPINFO biDst;
    HDC hdcDst = CreateCompatibleDC(0);
    HBITMAP bmpDst, bmpOld;
    INT nFormats, iPixelFormat, res, i;
    PIXELFORMATDESCRIPTOR pfd;
    HGLRC Context;
    RGBQUAD *dstBuffer = NULL;
    GLuint texObj;
    BYTE texData4[4][4] = {{0x00, 0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF, 0xFF}, {0x00, 0xFF, 0x00, 0xFF}, {0xFF, 0x00, 0x00, 0xFF}};
    BYTE texData3[4][3] = {{0x00, 0x00, 0x00}, {0x00, 0x00, 0xFF}, {0x00, 0xFF, 0x00}, {0xFF, 0x00, 0x00}};

    memset(&biDst, 0, sizeof(BITMAPINFO));
    biDst.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    biDst.bmiHeader.biWidth = 2;
    biDst.bmiHeader.biHeight = -2;
    biDst.bmiHeader.biPlanes = 1;
    biDst.bmiHeader.biBitCount = 32;
    biDst.bmiHeader.biCompression = BI_RGB;

    bmpDst = CreateDIBSection(0, &biDst, DIB_RGB_COLORS, (void**)&dstBuffer, NULL, 0);

    bmpOld = SelectObject(hdcDst, bmpDst);

    /* Choose a pixel format */
    nFormats = DescribePixelFormat(hdcDst, 0, 0, NULL);
    for(i=1; i<=nFormats; i++)
    {
        memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
        DescribePixelFormat(hdcDst, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

        if((pfd.dwFlags & PFD_DRAW_TO_BITMAP) &&
           (pfd.dwFlags & PFD_SUPPORT_OPENGL) &&
           (pfd.cColorBits == 32) &&
           (pfd.cAlphaBits == 8) )
        {
            iPixelFormat = i;
            break;
        }
    }

    ok(pfd.dwFlags & PFD_GENERIC_FORMAT, "We found a pixel format for drawing to bitmap which is not generic !\n");
    ok (iPixelFormat >= 1 && iPixelFormat <= nFormats, "Could not find a suitable pixel format.\n");
    res = SetPixelFormat(hdcDst, iPixelFormat, &pfd);
    ok (res != 0, "SetPixelFormat failed.\n");
    Context = wglCreateContext(hdcDst);
    ok(Context != NULL, "We failed to create a GL context.\n");
    wglMakeCurrent(hdcDst, Context);
    
    /* put us in orthogonal projection*/
    glMatrixMode(GL_PROJECTION);
    glOrtho(0, 2, 0, 2, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    /* Create a texture */
    glGenTextures(1, &texObj);
    ok(texObj != 0, "glGenTextures failed\n");
    
    /* load RGBA data there */
    glBindTexture(GL_TEXTURE_2D, texObj);
    ok(!glGetError(), "glBindTexture errored\n");
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData4 );
    ok(!glGetError(), "glTexImage2D errored\n");
    
    /* Draw it on our bitmap */
    ZeroMemory(dstBuffer, 4*sizeof(RGBQUAD));
    glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, texObj);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0); glVertex2i(0, 0);
    glTexCoord2i(0, 1); glVertex2i(0, 2);
    glTexCoord2i(1, 1); glVertex2i(2, 2);
    glTexCoord2i(1, 0); glVertex2i(2, 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFlush();
    
    ok_color_channel(dstBuffer[0].rgbBlue, 0x00); ok_color_channel(dstBuffer[0].rgbGreen, 0x00); ok_color_channel(dstBuffer[0].rgbRed, 0x00);
    ok_color_channel(dstBuffer[1].rgbBlue, 0xFF); ok_color_channel(dstBuffer[1].rgbGreen, 0x00); ok_color_channel(dstBuffer[1].rgbRed, 0x00);
    ok_color_channel(dstBuffer[2].rgbBlue, 0x00); ok_color_channel(dstBuffer[2].rgbGreen, 0xFF); ok_color_channel(dstBuffer[2].rgbRed, 0x00);
    ok_color_channel(dstBuffer[3].rgbBlue, 0x00); ok_color_channel(dstBuffer[3].rgbGreen, 0x00); ok_color_channel(dstBuffer[3].rgbRed, 0xFF);
        
    /* load BGRA data now */
    ZeroMemory(dstBuffer, 4*sizeof(RGBQUAD));
    glBindTexture(GL_TEXTURE_2D, texObj);
    ok(!glGetError(), "glBindTexture errored\n");
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, texData4 );
    ok(!glGetError(), "glTexImage2D errored\n");

    glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, texObj);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0); glVertex2i(0, 0);
    glTexCoord2i(0, 1); glVertex2i(0, 2);
    glTexCoord2i(1, 1); glVertex2i(2, 2);
    glTexCoord2i(1, 0); glVertex2i(2, 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFlush();

    ok_color_channel(dstBuffer[0].rgbBlue, 0x00); ok_color_channel(dstBuffer[0].rgbGreen, 0x00); ok_color_channel(dstBuffer[0].rgbRed, 0x00);
    ok_color_channel(dstBuffer[1].rgbBlue, 0x00); ok_color_channel(dstBuffer[1].rgbGreen, 0x00); ok_color_channel(dstBuffer[1].rgbRed, 0xFF);
    ok_color_channel(dstBuffer[2].rgbBlue, 0x00); ok_color_channel(dstBuffer[2].rgbGreen, 0xFF); ok_color_channel(dstBuffer[2].rgbRed, 0x00);
    ok_color_channel(dstBuffer[3].rgbBlue, 0xFF); ok_color_channel(dstBuffer[3].rgbGreen, 0x00); ok_color_channel(dstBuffer[3].rgbRed, 0x00);

    /* load "3" data now, mimicking CORE-16221 */
    ZeroMemory(dstBuffer, 4*sizeof(RGBQUAD));
    glBindTexture(GL_TEXTURE_2D, texObj);
    ok(!glGetError(), "glBindTexture errored\n");
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, 3, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, texData3 );
    ok(!glGetError(), "glTexImage2D errored\n");

    glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, texObj);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2i(0, 0); glVertex2i(0, 0);
    glTexCoord2i(0, 1); glVertex2i(0, 2);
    glTexCoord2i(1, 1); glVertex2i(2, 2);
    glTexCoord2i(1, 0); glVertex2i(2, 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFlush();

    ok_color_channel(dstBuffer[0].rgbBlue, 0x00); ok_color_channel(dstBuffer[0].rgbGreen, 0x00); ok_color_channel(dstBuffer[0].rgbRed, 0x00);
    ok_color_channel(dstBuffer[1].rgbBlue, 0xFF); ok_color_channel(dstBuffer[1].rgbGreen, 0x00); ok_color_channel(dstBuffer[1].rgbRed, 0x00);
    ok_color_channel(dstBuffer[2].rgbBlue, 0x00); ok_color_channel(dstBuffer[2].rgbGreen, 0xFF); ok_color_channel(dstBuffer[2].rgbRed, 0x00);
    ok_color_channel(dstBuffer[3].rgbBlue, 0x00); ok_color_channel(dstBuffer[3].rgbGreen, 0x00); ok_color_channel(dstBuffer[3].rgbRed, 0x00);

    /* cleanup */
    wglDeleteContext(Context);
    SelectObject(hdcDst, bmpOld);
    DeleteDC(hdcDst);
}
