__version__ = '0.1'

import os
import posixpath
import BaseHTTPServer
import urllib
import cgi
import shutil
import mimetypes
import SocketServer
import threading
import socket
import struct
try:
  from cStringIO import StringIO
except ImportError:
  from StringIO import StringIO

from subprocess import Popen, PIPE

class ECPRestHandler(BaseHTTPServer.BaseHTTPRequestHandler):

  server_version = "PeerConnectionTestHTTP/" + __version__

  def do_GET(self):
    print 'do_GET: ' + self.path
    f = self.send_head()
    if f:
      self.copyfile(f, self.wfile)
      f.close()

  def do_HEAD(self):
    print 'do_HEAD: ' + self.path
    f = self.send_head()
    if f:
      f.close()

#  def do_PUT(self):
#    print 'do_PUT: ' + self.path
#    if self.path == '/launch/99999':
#      self.send_response(200)
#    else:
#      self.send_response(500)
#    self.end_headers()

  def do_POST(self):
    print 'do_POST: ' + self.path
    if self.path == '/launch/99999':
      pc = Popen(['../glplayer',])
      self.send_response(200)
    else:
      self.send_response(500)
    self.end_headers()

  def send_head(self):
    path = self.translate_path(self.path)
    f = None
    if os.path.isdir(path):
      if not self.path.endswith('/'):
        # redirect browser - doing basically what apache does
        self.send_response(301)
        self.send_header("Location", self.path + "/")
        self.end_headers()
        return None
      for index in "index.xml", "index.html", "index.htm":
        index = os.path.join(path, index)
        if os.path.exists(index):
          path = index
          break
        else:
          self.send_error(404, "File not found")
          return None
    ctype = self.guess_type(path)
    try:
      # Always read in binary mode. Opening files in text mode may cause
      # newline translations, making the actual size of the content
      # transmitted *less* than the content-length!
      f = open(path, 'rb')
    except IOError:
      self.send_error(404, "File not found")
      return None
    self.send_response(200)
    self.send_header("Content-type", ctype)
    fs = os.fstat(f.fileno())
    self.send_header("Content-Length", str(fs[6]))
    self.send_header("Last-Modified", self.date_time_string(fs.st_mtime))
    self.end_headers()
    return f

  def translate_path(self, path):
    # abandon query parameters
    path = path.split('?',1)[0]
    path = path.split('#',1)[0]
    path = posixpath.normpath(urllib.unquote(path))
    words = path.split('/')
    words = filter(None, words)
    path = os.getcwd()
    for word in words:
      drive, word = os.path.splitdrive(word)
      head, word = os.path.split(word)
      if word in (os.curdir, os.pardir): continue
      path = os.path.join(path, word)
    return path

  def copyfile(self, source, outputfile):
    shutil.copyfileobj(source, outputfile)

  def guess_type(self, path):
    base, ext = posixpath.splitext(path)
    if ext in self.extensions_map:
      return self.extensions_map[ext]
    ext = ext.lower()
    if ext in self.extensions_map:
      return self.extensions_map[ext]
    else:
      return self.extensions_map['']

  if not mimetypes.inited:
    mimetypes.init() # try to read system mime.types
  extensions_map = mimetypes.types_map.copy()
  extensions_map.update({
    '': 'application/octet-stream', # Default
    '.py': 'text/plain',
    '.c': 'text/plain',
    '.h': 'text/plain',
    })

reply = """HTTP/1.1 200 OK
Cache-Control: max-age=300
ST: roku:ecp
Location: http://""" + socket.gethostbyname(socket.gethostname()) + """:8060/
USN: uuid:roku:ecp:P0A070000007

"""

HOST_NAME = ''
PORT_NUMBER=8060

def ssdpWorker():
  print "Worker thread"
  MCAST_GRP = "239.255.255.250"
  MCAST_PORT = 1900

  insock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
  insock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
  insock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
  insock.bind((MCAST_GRP, MCAST_PORT))
  mreq = struct.pack("4sl", socket.inet_aton(MCAST_GRP), socket.INADDR_ANY)
  insock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

  outsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
  outsock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)

  while True:
    packet = insock.recv(10240)
#    print "recv->"
#    print packet
    values = packet.split('\r\n')
    if len(values) > 2 and values[0].find('M-SEARCH') == 0:
      for line in values[1:]:
        if line == 'ST: roku:ecp':
          print "send->"
          print reply
          outsock.sendto(reply, (MCAST_GRP, MCAST_PORT))

t = threading.Thread(target=ssdpWorker)
t.setDaemon(True)
t.start()

server_class = BaseHTTPServer.HTTPServer
httpd = server_class((HOST_NAME, PORT_NUMBER), ECPRestHandler)

print "Starting Simulator"

try:
  while True:
    httpd.handle_request()
except KeyboardInterrupt:
  pass

print "\nEXIT"
httpd.server_close()
