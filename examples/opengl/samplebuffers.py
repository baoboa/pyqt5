#!/usr/bin/env python


#############################################################################
##
## Copyright (C) 2013 Riverbank Computing Limited.
## Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
## All rights reserved.
##
## This file is part of the examples of PyQt.
##
## $QT_BEGIN_LICENSE:BSD$
## You may use this file under the terms of the BSD license as follows:
##
## "Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are
## met:
##   * Redistributions of source code must retain the above copyright
##     notice, this list of conditions and the following disclaimer.
##   * Redistributions in binary form must reproduce the above copyright
##     notice, this list of conditions and the following disclaimer in
##     the documentation and/or other materials provided with the
##     distribution.
##   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
##     the names of its contributors may be used to endorse or promote
##     products derived from this software without specific prior written
##     permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
## A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
## OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
## SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
## LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
## DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
## THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
## $QT_END_LICENSE$
##
#############################################################################


import sys
import math

from PyQt5.QtCore import Qt
from PyQt5.QtGui import QColor
from PyQt5.QtWidgets import QApplication, QMessageBox
from PyQt5.QtOpenGL import QGL, QGLFormat, QGLWidget

try:
    from OpenGL import GL
except ImportError:
    app = QApplication(sys.argv)
    QMessageBox.critical(None, "OpenGL samplebuffers",
            "PyOpenGL must be installed to run this example.")
    sys.exit(1)


class GLWidget(QGLWidget):
    GL_MULTISAMPLE = 0x809D
    rot = 0.0

    def __init__(self, parent):
        super(GLWidget, self).__init__(QGLFormat(QGL.SampleBuffers), parent)

        self.list_ = []

        self.startTimer(40)
        self.setWindowTitle("Sample Buffers")

    def initializeGL(self):
        GL.glMatrixMode(GL.GL_PROJECTION)
        GL.glLoadIdentity()
        GL.glOrtho( -.5, .5, .5, -.5, -1000, 1000)
        GL.glMatrixMode(GL.GL_MODELVIEW)
        GL.glLoadIdentity()
        GL.glClearColor(1.0, 1.0, 1.0, 1.0)

        self.makeObject()

    def resizeGL(self, w, h):
        GL.glViewport(0, 0, w, h)

    def paintGL(self):
        GL.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT)

        GL.glMatrixMode(GL.GL_MODELVIEW)
        GL.glPushMatrix()
        GL.glEnable(GLWidget.GL_MULTISAMPLE)
        GL.glTranslatef( -0.25, -0.10, 0.0)
        GL.glScalef(0.75, 1.15, 0.0)
        GL.glRotatef(GLWidget.rot, 0.0, 0.0, 1.0)
        GL.glCallList(self.list_)
        GL.glPopMatrix()

        GL.glPushMatrix()
        GL.glDisable(GLWidget.GL_MULTISAMPLE)
        GL.glTranslatef(0.25, -0.10, 0.0)
        GL.glScalef(0.75, 1.15, 0.0)
        GL.glRotatef(GLWidget.rot, 0.0, 0.0, 1.0)
        GL.glCallList(self.list_)
        GL.glPopMatrix()

        GLWidget.rot += 0.2

        self.qglColor(Qt.black)
        self.renderText(-0.35, 0.4, 0.0, "Multisampling enabled")
        self.renderText(0.15, 0.4, 0.0, "Multisampling disabled")

    def timerEvent(self, event):
        self.update()

    def makeObject(self):
        trolltechGreen = QColor.fromCmykF(0.40, 0.0, 1.0, 0.0)
        NumSectors = 15
        x1 = +0.06
        y1 = -0.14
        x2 = +0.14
        y2 = -0.06
        x3 = +0.08
        y3 = +0.00
        x4 = +0.30
        y4 = +0.22

        self.list_ = GL.glGenLists(1)
        GL.glNewList(self.list_, GL.GL_COMPILE)

        for i in range(NumSectors):
            angle1 = float((i * 2 * math.pi) / NumSectors)
            x5 = 0.30 * math.sin(angle1)
            y5 = 0.30 * math.cos(angle1)
            x6 = 0.20 * math.sin(angle1)
            y6 = 0.20 * math.cos(angle1)

            angle2 = float(((i + 1) * 2 * math.pi) / NumSectors)
            x7 = 0.20 * math.sin(angle2)
            y7 = 0.20 * math.cos(angle2)
            x8 = 0.30 * math.sin(angle2)
            y8 = 0.30 * math.cos(angle2)

            self.qglColor(trolltechGreen)
            self.quad(GL.GL_QUADS, x5, y5, x6, y6, x7, y7, x8, y8)
            self.qglColor(Qt.black)
            self.quad(GL.GL_LINE_LOOP, x5, y5, x6, y6, x7, y7, x8, y8)

        self.qglColor(trolltechGreen)
        self.quad(GL.GL_QUADS, x1, y1, x2, y2, y2, x2, y1, x1)
        self.quad(GL.GL_QUADS, x3, y3, x4, y4, y4, x4, y3, x3)

        self.qglColor(Qt.black)
        self.quad(GL.GL_LINE_LOOP, x1, y1, x2, y2, y2, x2, y1, x1)
        self.quad(GL.GL_LINE_LOOP, x3, y3, x4, y4, y4, x4, y3, x3)

        GL.glEndList()

    def quad(self, primitive, x1, y1, x2, y2, x3, y3, x4, y4):
        GL.glBegin(primitive)

        GL.glVertex2d(x1, y1)
        GL.glVertex2d(x2, y2)
        GL.glVertex2d(x3, y3)
        GL.glVertex2d(x4, y4)

        GL.glEnd()


if __name__ == '__main__':

    app = QApplication(sys.argv)

    f = QGLFormat.defaultFormat()
    f.setSampleBuffers(True)
    QGLFormat.setDefaultFormat(f)

    if not QGLFormat.hasOpenGL():
        QMessageBox.information(None, "OpenGL samplebuffers",
                "This system does not support OpenGL.")
        sys.exit(0)

    widget = GLWidget(None)

    if not widget.format().sampleBuffers():
        QMessageBox.information(None, "OpenGL samplebuffers",
                "This system does not have sample buffer support.")
        sys.exit(0)

    widget.resize(640, 480)
    widget.show()

    sys.exit(app.exec_())
