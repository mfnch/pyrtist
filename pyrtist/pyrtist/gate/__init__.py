queue = None

class GUIGate(object):
    def __init__(self):
        self._gui_tx_pipe = None

    def move(self, **kwargs):
        for name, value in kwargs.items():
            self._gui_tx_pipe.send(('script_move_point',
                                    (name,) + tuple(value)))
