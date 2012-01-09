#! /usr/bin/python

'''
Oven controller GUI - works in conjunction with external oven control hardware.

Copyright (c) 2011, Daniel Strother < http://danstrother.com/ >
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
  - Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  - Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  - The name of the author may not be used to endorse or promote products
    derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
'''

# depends on python-qt4, python-qwt5-qt4, python-serial
from PyQt4 import QtGui, QtCore
import PyQt4.Qwt5 as Qwt
import math
import sys
import serial
import time

class OvenMsg():
    """Class representing a single status message sent from the oven control hardware."""

    def __init__(self):
        """Sets sane default message contents."""
        self.state      = 'idle'
        self.time       = 0.0
        self.target     = 0.0
        self.sense_t    = 0.0
        self.sense_b    = 0.0
        self.cmd        = 0.0
        self.cmd_t      = 0.0
        self.cmd_b      = 0.0

    def parse(self,msg):
        """Parses message contents from a comma-separated string.
        
        On microcontroller, message is generated with the C code:
        sprintf_P(tx_msg,PSTR("%s,%u,%d,%d,%d,%u,%u,%u\\n"),
            state_names[state],
            time,
            target,
            temp_t,
            temp_b,
            cmd,
            cmd_t,
            cmd_b);
        
        This parses that."""

        m               = msg.split(',')
        if(len(m) != 8):
            return 0

        self.state      = m.pop(0)
        self.time       = int(m.pop(0))*0.25
        self.target     = int(m.pop(0))*0.25
        self.sense_t    = int(m.pop(0))*0.25
        self.sense_b    = int(m.pop(0))*0.25
        self.cmd        = int(m.pop(0))/255.0
        self.cmd_t      = int(m.pop(0))/255.0
        self.cmd_b      = int(m.pop(0))/255.0

        return 1


class OvenCommThread(QtCore.QThread):
    """Thread class responsible for asynchronously receiving messages from oven controller on behalf of OvenComm instance."""

    def __init__(self,parent=None):
        """Constructor; requires an OvenComm parent."""
        super(OvenCommThread,self).__init__(parent)
        self.running = True
        self.p = parent

    def __del__(self):
        """Tells thread to stop, then waits (a little while) for thread to terminate"""
        self.stop()
        self.wait()

    def run(self):
        """Triggers newMessage signal in parent OvenComm instance whenever a message is received."""
        while(self.running):
            msg = OvenMsg()
            if(msg.parse(self.p.s.readline()) and self.running):
                self.p.trigger_newMessage(msg)

    def stop(self):
        """Tells comm thread to stop running (eventually)."""
        self.running = False


class OvenComm(QtCore.QObject):
    """Class that encapsulates all serial communication with oven controller hardware."""

    newMessage = QtCore.pyqtSignal('PyQt_PyObject')     # PyQt_PyObject required for passing non-C++ types through Qt signals

    newSenseT = QtCore.pyqtSignal(float)
    newSenseB = QtCore.pyqtSignal(float)

    def __init__(self,parent=None,port='/dev/ttyUSB000'):
        """Opens specified serial port and starts comm thread."""

        super(OvenComm,self).__init__(parent)
        
        self.thread = None
        self.s = None

        self.s = serial.Serial(port=port,timeout=0.7)

        # clear any stale data that may have been buffered prior to program start
        time.sleep(0.5)
        self.s.flushInput()
        self.s.readline()

        self.thread = OvenCommThread(self)
        self.thread.start()

        self.v_cmd_t = 0
        self.v_cmd_b = 0

    def __del__(self):
        """Terminates comm thread and closes serial port."""

        self.newSenseT.disconnect()
        self.newSenseB.disconnect()
        self.newMessage.disconnect()

        if(self.thread):
            self.thread.stop()
            self.thread.wait()  # don't close serial port until after listening thread has stopped
        if(self.s):
            self.s.close()

    def trigger_newMessage(self,msg):
        """Callback for triggering Qt signals on receipt of new message by OvenCommThread."""

        self.newSenseT.emit(msg.sense_t)
        self.newSenseB.emit(msg.sense_b)
        self.newMessage.emit(msg)

    def go(self):
        """Callback for Go button - resets controller and starts reflow operation."""
        self.s.write("reset\ngo\n")

    def reset(self):
        """Callback for Reset button - just resets controller (stops ongoing reflow operation)."""
        self.s.write("reset\n")

    def pause(self):
        """Callback for Pause button - puts controller into pause state (profile time does not advance)."""
        self.s.write("pause\n")

    def resume(self):
        """Callback for Resume button - resumes controller after pause (profile time resumes advancing)."""
        self.s.write("resume\n")

    def manual(self,m):
        """Callback for Manual check-box - when enabled, controller's PID loop is bypassed."""
        if(m):
            self.s.write("manual: 1\n")
        else:
            self.s.write("manual: 0\n")

    def cmd_t(self,t):
        """Callback for Command-Top slider - controls power to top heating element."""
        self.v_cmd_t = int(t*255.0/100.0)
        self.cmd()

    def cmd_b(self,b):
        """Callback for Command-Bottom slider - controls power to bottom heating element."""
        self.v_cmd_b = int(b*255.0/100.0)
        self.cmd()

    def cmd(self):
        """Sends updated manual power commands to controller (after clamping to safe ranges)."""
        if(self.v_cmd_t < 0):
            self.v_cmd_t = 0
        if(self.v_cmd_t > 255):
            self.v_cmd_t = 255
        if(self.v_cmd_b < 0):
            self.v_cmd_b = 0
        if(self.v_cmd_b > 255):
            self.v_cmd_b = 255
        print >> self.s, "cmd: %d, %d" % (self.v_cmd_t,self.v_cmd_b)

    def target(self,t):
        """Callback for Target slider - controls target temperature when in Idle state."""
        t = int(t*4)
        if(t < 0):
            t = 0
        if(t > 4095):
            t = 4095
        print >> self.s, "target: %d" % (t)


class OvenLogger():
    """Class for logging oven status messages to CSV files."""

    def __init__(self,comm):
        """Connects newMessage signal handler to OvenComm instance."""
        self.comm = comm
        self.prevstate = 'idle'
        self.f = None
        self.time_offset = 0.0
        self.comm.newMessage.connect(self.log_message)

    def __del__(self):
        """Closes open log file."""
        if(self.f):
            self.f.close()

    def start_new_file(self):
        """Starts a new time-stamped log file and inserts column headers."""
        if(self.f):
            self.f.close()
        filename = time.strftime('ovenlog_%Y%m%d%H%M%S.csv')
        self.f = open(filename,'a')
        print >> self.f, "state,time,target,sense_t,sense_b,cmd,cmd_t,cmd_b"

    def log_message(self,msg):
        """Callback for newMessage signals - writes messages to log file.

        Opens a new log file when transitioning from a non-idle state to idle."""

        if(not self.f or (msg.state == 'idle' and self.prevstate != 'idle')):
            # when transitioning to idle state (or if log file not already open),
            # open a new log file
            self.start_new_file()

            # time_offset is used to make all log files start at time 0
            # (controller time always increments, and never resets)
            self.time_offset = msg.time

        self.prevstate = msg.state
        print >> self.f, "%s,%f,%f,%f,%f,%f,%f,%f" % (msg.state,msg.time-self.time_offset,msg.target,msg.sense_t,msg.sense_b,msg.cmd,msg.cmd_t,msg.cmd_b)


class OvenPlot(Qwt.QwtPlot):
    """Common base-class for plotting oven data."""

    def __init__(self,comm,parent=None):
        """Connents newMessage handler to OvenComm instance and sets up common plot format."""
        super(OvenPlot,self).__init__(parent)

        self.comm = comm
        
        self.setCanvasBackground(QtCore.Qt.white)

        # plot grid
        grid = Qwt.QwtPlotGrid()
        pen = QtGui.QPen(QtCore.Qt.DotLine)
        pen.setColor(QtCore.Qt.black)
        pen.setWidth(0)
        grid.setPen(pen)
        grid.attach(self)

        # plot legend and x-axis (y-axis handled in derived classes)
        self.insertLegend(Qwt.QwtLegend(), Qwt.QwtPlot.BottomLegend)
        self.setAxisTitle(Qwt.QwtPlot.xBottom, "Time (seconds)")

        self.times = []
        self.time_offset = 0.0
        self.max_idle = (120*4)
        self.prevstate = 'idle'

        self.reset_plot()
        
        comm.newMessage.connect(self.newMessage_handler)

    def newMessage_handler(self,msg):
        """Callback for newMessage signals - adds new data to plot."""

        if(msg.state == 'idle' and self.prevstate != 'idle'):
            # when transitioning to idle state, start with a clean plot
            self.time_offset = msg.time
            self.reset_plot()

        self.prevstate = msg.state
        self.update_plot(msg)

    def update_plot(self,msg):
        """Adds new data to plot, and prunes old data."""

        self.times.append(msg.time-self.time_offset)

        if(msg.state == 'idle'):
            # when idle, newest entry is always at time 0
            # (so transition to non-idle state begins at time 0)
            diff = msg.time - self.time_offset
            self.time_offset = msg.time
            self.times = [t - diff for t in self.times]

        if(msg.state == 'idle' and len(self.times) > self.max_idle):
            # when idle, limit visible window of time to max_idle entries
            self.times.pop(0)

    def reset_plot(self):
        """Resets plot to empty (no data) state."""
        self.times = []


class OvenTempPlot(OvenPlot):
    """Class for plotting temperatures in oven."""

    def __init__(self,comm,parent=None):
        """Sets up OvenTempPlot-specific formatting."""

        super(OvenTempPlot,self).__init__(comm,parent)

        self.setTitle("Temperatures")
        self.setAxisTitle(Qwt.QwtPlot.yLeft, "Temperature (degrees celsius)")

        self.c_target = Qwt.QwtPlotCurve("Target")
        self.c_sense_t = Qwt.QwtPlotCurve("Top")
        self.c_sense_b = Qwt.QwtPlotCurve("Bottom")

        self.c_target.attach(self)
        self.c_sense_t.attach(self)
        self.c_sense_b.attach(self)
        
        pen = QtGui.QPen()
        pen.setColor(QtCore.Qt.black)
        pen.setWidth(4)
       
        self.c_target.setPen(pen)
        self.c_sense_t.setPen(QtGui.QPen(QtCore.Qt.red))
        self.c_sense_b.setPen(QtGui.QPen(QtCore.Qt.blue))

    def update_plot(self,msg):
        """Adds new data to plot, and prunes old data."""

        super(OvenTempPlot,self).update_plot(msg)

        self.target.append(msg.target)
        self.sense_t.append(msg.sense_t)
        self.sense_b.append(msg.sense_b)

        if(msg.state == 'idle' and len(self.target) > self.max_idle):
            # when idle, limit visible window of time to max_idle entries
            self.target.pop(0)
            self.sense_t.pop(0)
            self.sense_b.pop(0)

        self.c_target.setData(self.times,self.target)
        self.c_sense_t.setData(self.times,self.sense_t)
        self.c_sense_b.setData(self.times,self.sense_b)

        # actually update the plot
        self.replot()

    def reset_plot(self):
        """Resets plot to empty (no data) state."""

        super(OvenTempPlot,self).reset_plot()

        self.target = []
        self.sense_t = []
        self.sense_b = []


class OvenCommandPlot(OvenPlot):
    """Class for plotting power commands to oven."""

    def __init__(self,comm,parent=None):
        """Sets up OvenCommandPlot-specific formatting."""

        super(OvenCommandPlot,self).__init__(comm,parent)

        self.setTitle("Commands")
        self.setAxisTitle(Qwt.QwtPlot.yLeft, "Command")

        self.c_cmd = Qwt.QwtPlotCurve("Command")
        self.c_cmd_t = Qwt.QwtPlotCurve("Top")
        self.c_cmd_b = Qwt.QwtPlotCurve("Bottom")

        self.c_cmd.attach(self)
        self.c_cmd_t.attach(self)
        self.c_cmd_b.attach(self)
        
        pen = QtGui.QPen()
        pen.setColor(QtCore.Qt.black)
        pen.setWidth(4)
       
        self.c_cmd.setPen(pen)
        self.c_cmd_t.setPen(QtGui.QPen(QtCore.Qt.red))
        self.c_cmd_b.setPen(QtGui.QPen(QtCore.Qt.blue))

    def update_plot(self,msg):
        """Adds new data to plot, and prunes old data."""

        super(OvenCommandPlot,self).update_plot(msg)

        self.cmd.append(msg.cmd*100.0)
        self.cmd_t.append(msg.cmd_t*100.0)
        self.cmd_b.append(msg.cmd_b*100.0)

        if(msg.state == 'idle' and len(self.cmd) > self.max_idle):
            # when idle, limit visible window of time to max_idle entries
            self.cmd.pop(0)
            self.cmd_t.pop(0)
            self.cmd_b.pop(0)

        self.c_cmd.setData(self.times,self.cmd)
        self.c_cmd_t.setData(self.times,self.cmd_t)
        self.c_cmd_b.setData(self.times,self.cmd_b)

        # actually update the plot
        self.replot()

    def reset_plot(self):
        """Resets plot to empty (no data) state."""

        super(OvenCommandPlot,self).reset_plot()

        self.cmd = []
        self.cmd_t = []
        self.cmd_b = []


class OvenPlots(QtGui.QWidget):
    """Widget containing all GUI plots for oven controller."""

    def __init__(self,comm,parent=None):
        """Creates plots and assigns layouts."""

        super(OvenPlots,self).__init__(parent)

        self.temps = OvenTempPlot(comm,parent=self)
        self.commands = OvenCommandPlot(comm,parent=self)

        l = QtGui.QVBoxLayout()
        l.addWidget(self.temps)
        l.addWidget(self.commands)

        self.setLayout(l)


class OvenControls(QtGui.QWidget):
    """Widget containing all GUI controls for oven controller."""

    def __init__(self,comm,parent=None):
        """Creates controls and assigns callbacks/layouts."""

        super(OvenControls,self).__init__(parent)

        # slider for idle-state target temperature
        self.target_slider = QtGui.QSlider(QtCore.Qt.Vertical,self)
        self.target_slider.setMinimum(0)
        self.target_slider.setMaximum(250)
        self.target_slider.valueChanged.connect(comm.target)

        # slider for manual-mode top element power
        self.cmdt_slider = QtGui.QSlider(QtCore.Qt.Vertical,self)
        self.cmdt_slider.setMinimum(0)
        self.cmdt_slider.setMaximum(100)
        self.cmdt_slider.valueChanged.connect(comm.cmd_t)

        # slider for manual-mode bottom element power
        self.cmdb_slider = QtGui.QSlider(QtCore.Qt.Vertical,self)
        self.cmdb_slider.setMinimum(0)
        self.cmdb_slider.setMaximum(100)
        self.cmdb_slider.valueChanged.connect(comm.cmd_b)

        # sliders adjacent to each other
        sliders = QtGui.QHBoxLayout()
        sliders.addWidget(self.target_slider)
        sliders.addWidget(self.cmdt_slider)
        sliders.addWidget(self.cmdb_slider)

        # LCDs indicating sensed temperatures
        self.senset_lcd = QtGui.QLCDNumber(self)
        self.senseb_lcd = QtGui.QLCDNumber(self)
        self.senset_lcd.setSegmentStyle(QtGui.QLCDNumber.Flat)
        self.senseb_lcd.setSegmentStyle(QtGui.QLCDNumber.Flat)
        comm.newSenseT.connect(self.senset_lcd.display)
        comm.newSenseB.connect(self.senseb_lcd.display)

        self.manual_checkbox = QtGui.QCheckBox("Manual",self)
        self.manual_checkbox.stateChanged.connect(comm.manual)

        self.reset_button = QtGui.QPushButton("Reset",self)
        self.reset_button.clicked.connect(comm.reset)

        self.go_button = QtGui.QPushButton("Go",self)
        self.go_button.clicked.connect(comm.go)

        self.pause_button = QtGui.QPushButton("Pause",self)
        self.pause_button.clicked.connect(comm.pause)

        self.resume_button = QtGui.QPushButton("Resume",self)
        self.resume_button.clicked.connect(comm.resume)

        # overall column layout
        l = QtGui.QVBoxLayout()
        l.addWidget(self.senset_lcd)
        l.addWidget(self.senseb_lcd)
        l.addLayout(sliders)
        l.addWidget(self.manual_checkbox)
        l.addWidget(self.reset_button)
        l.addWidget(self.go_button)
        l.addWidget(self.pause_button)
        l.addWidget(self.resume_button)

        self.setLayout(l)


class OvenMain(QtGui.QWidget):
    """Widget containing all oven controller GUI elements."""

    def __init__(self,comm,parent=None):
        """Creates Plots and Controls, and assigns layout."""

        super(OvenMain,self).__init__(parent)
        
        self.plots      = OvenPlots(comm,parent=self)
        self.controls   = OvenControls(comm,parent=self)

        l = QtGui.QHBoxLayout()
        l.addWidget(self.plots)
        l.addWidget(self.controls)
        self.setLayout(l)


class OvenCon(QtGui.QMainWindow):
    """Main window for oven controller GUI."""

    def __init__(self,port):
        """Sets up main windows and starts application execution."""

        super(OvenCon,self).__init__()
        
        self.resize(1000,600)
        self.setWindowTitle('Reflow Controller')

        # connect to controller hardware
        self.comm       = OvenComm(parent=self,port=port)

        # log controller status to disk
        self.logger     = OvenLogger(self.comm)

        # create GUI
        self.main       = OvenMain(self.comm,parent=self)

        # associate GUI with window
        self.setCentralWidget(self.main)

if __name__ == '__main__':
    # start GUI application when invoked stand-alone
    app = QtGui.QApplication(sys.argv)
    port = '/dev/ttyUSB000';
    if(len(sys.argv)>1 and sys.argv[1]):
        # get serial port from command line
        port = sys.argv[1]
    try:
        qb = OvenCon(port)
    except serial.SerialException as se:
        print "failed to open serial port - \"%s\"" % (se)
        sys.exit(1)
    else:
        qb.show()
        sys.exit(app.exec_())

