# -*- coding: utf-8 -*-
"""
Created on Sun May 28 19:44:08 2017
IDE : Spyder
ANACONDA Distribution
@author: LALIT ARORA
"""

from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QMessageBox

import get_ports
import serial

class Ui_ElectKitv1(object):
    def setupUi(self, ElectKitv1):
        ElectKitv1.setObjectName("ElectKitv1")
        ElectKitv1.resize(616, 443)
        ElectKitv1.setMaximumSize(QtCore.QSize(616, 443))
        self.label_6 = QtWidgets.QLabel(ElectKitv1)
        self.label_6.setGeometry(QtCore.QRect(100, 260, 47, 13))
        self.label_6.setObjectName("label_6")
        self.sensor4 = QtWidgets.QLabel(ElectKitv1)
        self.sensor4.setGeometry(QtCore.QRect(220, 260, 47, 13))
        self.sensor4.setObjectName("sensor4")
        self.sensor4.hide()==True
        self.label_10 = QtWidgets.QLabel(ElectKitv1)
        self.label_10.setGeometry(QtCore.QRect(400, 200, 47, 13))
        self.label_10.setObjectName("label_10")
        self.pushButton_2 = QtWidgets.QPushButton(ElectKitv1)
        self.pushButton_2.setGeometry(QtCore.QRect(260, 380, 141, 23))
        self.pushButton_2.setObjectName("pushButton_2")
        self.pushButton_2.clicked.connect(self.fetch)
        self.sensor6 = QtWidgets.QLabel(ElectKitv1)
        self.sensor6.setGeometry(QtCore.QRect(220, 320, 47, 13))
        self.sensor6.setObjectName("sensor6")
        self.sensor6.hide()==True
        self.label_8 = QtWidgets.QLabel(ElectKitv1)
        self.label_8.setGeometry(QtCore.QRect(100, 320, 47, 13))
        self.label_8.setObjectName("label_8")
        self.comboBox = QtWidgets.QComboBox(ElectKitv1)
        self.comboBox.setGeometry(QtCore.QRect(300, 40, 69, 22))
        self.comboBox.setObjectName("comboBox")
        self.comboBox.addItems(ui.allports)
        self.pushButton = QtWidgets.QPushButton(ElectKitv1)
        self.pushButton.setGeometry(QtCore.QRect(400, 40, 111, 21))
        self.pushButton.setObjectName("pushButton")
        self.pushButton.clicked.connect(self.check)
        self.label_12 = QtWidgets.QLabel(ElectKitv1)
        self.label_12.setGeometry(QtCore.QRect(400, 260, 51, 16))
        self.label_12.setObjectName("label_12")
        self.label_3 = QtWidgets.QLabel(ElectKitv1)
        self.label_3.setGeometry(QtCore.QRect(100, 170, 47, 13))
        self.label_3.setObjectName("label_3")
        self.sensor10 = QtWidgets.QLabel(ElectKitv1)
        self.sensor10.setGeometry(QtCore.QRect(500, 260, 47, 13))
        self.sensor10.setObjectName("sensor10")
        self.sensor1 = QtWidgets.QLabel(ElectKitv1)
        self.sensor1.setGeometry(QtCore.QRect(220, 170, 47, 13))
        self.sensor1.setObjectName("sensor1")
        self.sensor5 = QtWidgets.QLabel(ElectKitv1)
        self.sensor5.setGeometry(QtCore.QRect(220, 290, 47, 13))
        self.sensor5.setObjectName("sensor5")
        self.label_2 = QtWidgets.QLabel(ElectKitv1)
        self.label_2.setGeometry(QtCore.QRect(80, 130, 61, 21))
        self.label_2.setObjectName("label_2")
        self.label_4 = QtWidgets.QLabel(ElectKitv1)
        self.label_4.setGeometry(QtCore.QRect(100, 200, 47, 13))
        self.label_4.setObjectName("label_4")
        self.label_11 = QtWidgets.QLabel(ElectKitv1)
        self.label_11.setGeometry(QtCore.QRect(400, 230, 47, 13))
        self.label_11.setObjectName("label_11")
        self.sensor2 = QtWidgets.QLabel(ElectKitv1)
        self.sensor2.setGeometry(QtCore.QRect(220, 200, 47, 13))
        self.sensor2.setObjectName("sensor2")
        self.sensor8 = QtWidgets.QLabel(ElectKitv1)
        self.sensor8.setGeometry(QtCore.QRect(500, 200, 47, 13))
        self.sensor8.setObjectName("sensor8")
        self.sensor9 = QtWidgets.QLabel(ElectKitv1)
        self.sensor9.setGeometry(QtCore.QRect(500, 230, 47, 13))
        self.sensor9.setObjectName("sensor9")
        self.label_5 = QtWidgets.QLabel(ElectKitv1)
        self.label_5.setGeometry(QtCore.QRect(100, 230, 47, 13))
        self.label_5.setObjectName("label_5")
        self.label_9 = QtWidgets.QLabel(ElectKitv1)
        self.label_9.setGeometry(QtCore.QRect(400, 170, 47, 13))
        self.label_9.setObjectName("label_9")
        self.label = QtWidgets.QLabel(ElectKitv1)
        self.label.setGeometry(QtCore.QRect(160, 40, 101, 20))
        font = QtGui.QFont()
        font.setPointSize(12)
        self.label.setFont(font)
        self.label.setObjectName("label")
        self.sensor7 = QtWidgets.QLabel(ElectKitv1)
        self.sensor7.setGeometry(QtCore.QRect(500, 170, 47, 13))
        self.sensor7.setObjectName("sensor7")
        self.label_7 = QtWidgets.QLabel(ElectKitv1)
        self.label_7.setGeometry(QtCore.QRect(100, 290, 47, 13))
        self.label_7.setObjectName("label_7")
        self.sensor12 = QtWidgets.QLabel(ElectKitv1)
        self.sensor12.setGeometry(QtCore.QRect(500, 320, 47, 13))
        self.sensor12.setObjectName("sensor12")
        self.sensor11 = QtWidgets.QLabel(ElectKitv1)
        self.sensor11.setGeometry(QtCore.QRect(500, 290, 47, 13))
        self.sensor11.setObjectName("sensor11")
        self.sensor3 = QtWidgets.QLabel(ElectKitv1)
        self.sensor3.setGeometry(QtCore.QRect(220, 230, 47, 13))
        self.sensor3.setObjectName("sensor3")
        self.label_13 = QtWidgets.QLabel(ElectKitv1)
        self.label_13.setGeometry(QtCore.QRect(400, 290, 51, 16))
        self.label_13.setObjectName("label_13")
        self.label_14 = QtWidgets.QLabel(ElectKitv1)
        self.label_14.setGeometry(QtCore.QRect(400, 320, 51, 16))
        self.label_14.setObjectName("label_14")
        self.sensor1.hide()==True
        self.sensor2.hide()==True
        self.sensor3.hide()==True
        self.sensor5.hide()==True
        self.sensor7.hide()==True
        self.sensor8.hide()==True
        self.sensor9.hide()==True
        self.sensor10.hide()==True
        self.sensor11.hide()==True
        self.sensor12.hide()==True
        self.retranslateUi(ElectKitv1)
        QtCore.QMetaObject.connectSlotsByName(ElectKitv1)

    def retranslateUi(self, ElectKitv1):
        _translate = QtCore.QCoreApplication.translate
        ElectKitv1.setWindowTitle(_translate("ElectKitv1", "EAK v1.0"))
        self.label_6.setText(_translate("ElectKitv1", "DEVICE 4"))
        self.sensor4.setText(_translate("ElectKitv1", "TextLabel"))
        self.label_10.setText(_translate("ElectKitv1", "DEVICE 8"))
        self.pushButton_2.setText(_translate("ElectKitv1", "GET DATA "))
        self.sensor6.setText(_translate("ElectKitv1", "TextLabel"))
        self.label_8.setText(_translate("ElectKitv1", "DEVICE 6"))
        self.pushButton.setText(_translate("ElectKitv1", "CHECK FOR DEVICES"))
        self.label_12.setText(_translate("ElectKitv1", "DEVICE 10"))
        self.label_3.setText(_translate("ElectKitv1", "DEVICE 1"))
        self.sensor10.setText(_translate("ElectKitv1", "TextLabel"))
        self.sensor1.setText(_translate("ElectKitv1", "TextLabel"))
        self.sensor5.setText(_translate("ElectKitv1", "TextLabel"))
        self.label_2.setText(_translate("ElectKitv1", "READINGS"))
        self.label_4.setText(_translate("ElectKitv1", "DEVICE 2"))
        self.label_11.setText(_translate("ElectKitv1", "DEVICE 9"))
        self.sensor2.setText(_translate("ElectKitv1", "TextLabel"))
        self.sensor8.setText(_translate("ElectKitv1", "TextLabel"))
        self.sensor9.setText(_translate("ElectKitv1", "TextLabel"))
        self.label_5.setText(_translate("ElectKitv1", "DEVICE 3"))
        self.label_9.setText(_translate("ElectKitv1", "DEVICE 7"))
        self.label.setText(_translate("ElectKitv1", "SELECT COM"))
        self.sensor7.setText(_translate("ElectKitv1", "TextLabel"))
        self.label_7.setText(_translate("ElectKitv1", "DEVICE 5"))
        self.sensor12.setText(_translate("ElectKitv1", "TextLabel"))
        self.sensor11.setText(_translate("ElectKitv1", "TextLabel"))
        self.sensor3.setText(_translate("ElectKitv1", "TextLabel"))
        self.label_13.setText(_translate("ElectKitv1", "DEVICE 11"))
        self.label_14.setText(_translate("ElectKitv1", "DEVICE 12"))
        
    
    def check(self):
        
        ui.portselected=self.comboBox.currentText()
        global ser
        ser=serial.Serial(ui.portselected,115200)
        while ser.inWaiting()==0:
            pass
        data=str(ser.readline())
        ls=data.split("  ")
        temp=str(ls[0])
        temp=temp[2:]
        ls[0]=temp
        n=len(ls)
        ls.pop(n-1)
        n=n-1
        self.s=str(n)+" Devices Connected.."
        print(self.s)
        w=QMessageBox()
        w.setText(self.s)
        w.setStandardButtons(QMessageBox.Ok)
        w.exec()
        
    def fetch(self):
        while ser.inWaiting()==0:
            pass
        data=str(ser.readline())
        ls=data.split("  ")
        temp=str(ls[0])
        temp=temp[2:]
        ls[0]=temp
        n=len(ls)
        ls.pop(n-1)
        n=n-1
        self.sensor1.setText(ls[0])
        self.sensor1.show()==True
        self.sensor2.setText(ls[1])
        self.sensor2.show()==True
        self.sensor3.setText(ls[2])
        self.sensor3.show()==True
        self.sensor4.setText(ls[3])
        self.sensor4.show()==True
        self.sensor5.setText(ls[4])
        self.sensor5.show()==True
        
if __name__ == "__main__":
    import sys
    app = QtWidgets.QApplication(sys.argv)
    ElectKitv1 = QtWidgets.QWidget()
    ui = Ui_ElectKitv1()
    ui.allports=get_ports.serial_ports()
    ui.portselected=""
    ui.setupUi(ElectKitv1)
    ElectKitv1.show()
    sys.exit(app.exec_())