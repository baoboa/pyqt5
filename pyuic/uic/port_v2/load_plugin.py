#############################################################################
##
## Copyright (c) 2015 Riverbank Computing Limited <info@riverbankcomputing.com>
## 
## This file is part of PyQt5.
## 
## This file may be used under the terms of the GNU General Public License
## version 3.0 as published by the Free Software Foundation and appearing in
## the file LICENSE included in the packaging of this file.  Please review the
## following information to ensure the GNU General Public License version 3.0
## requirements will be met: http://www.gnu.org/copyleft/gpl.html.
## 
## If you do not wish to use this file under the terms of the GPL version 3.0
## then you may purchase a commercial license.  For more information contact
## info@riverbankcomputing.com.
## 
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
#############################################################################


from ..exceptions import WidgetPluginError


def load_plugin(filename, plugin_globals, plugin_locals):
    """ Load the plugin from the given file.  Return True if the plugin was
    loaded, or False if it wanted to be ignored.  Raise an exception if there
    was an error.
    """

    plugin = open(filename, 'rU')

    try:
        exec(plugin.read(), plugin_globals, plugin_locals)
    except ImportError:
        return False
    except Exception, e:
        raise WidgetPluginError("%s: %s" % (e.__class__, str(e)))
    finally:
        plugin.close()

    return True
