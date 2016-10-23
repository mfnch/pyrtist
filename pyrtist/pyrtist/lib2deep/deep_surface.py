__all__ = ('DeepSurface',)

from .. import deepsurface


class ImageBufferWrapper(deepsurface.ImageBuffer):
    def __init__(self, *args):
        super(ImageBufferWrapper, self).__init__(*args)
        self.cairo_surface = None


class DeepSurface(object):
    def __init__(self, width, height, num_reusable_buffers=3):
        self.width = width
        self.height = height

        self.reusable_image_buffers = []
        self.reusable_depth_buffers = []
        self.num_reusable_buffers = num_reusable_buffers
        self.creation_limit = 20

        self.image_buffer = self.take_image_buffer()
        self.depth_buffer = self.take_depth_buffer()

    def take_image_buffer(self, clear=True):
        if len(self.reusable_image_buffers) > 0:
            ret = self.reusable_image_buffers.pop()
        else:
            if self.creation_limit < 1:
                print('Refuse to create buffer')
                return None
            self.creation_limit -= 1
            ret = deepsurface.ImageBuffer(self.width, self.height)
        if clear:
            ret.clear()
        return ret

    def take_depth_buffer(self, clear=True):
        if len(self.reusable_depth_buffers) > 0:
            ret = self.reusable_depth_buffers.pop()
        else:
            if self.creation_limit < 1:
                print('Refuse to create buffer')
                return None
            self.creation_limit -= 1
            ret = deepsurface.DepthBuffer(self.width, self.height)
        if clear:
            ret.clear()
        return ret

    def give_depth_buffer(self, depth_buffer):
        if len(self.reusable_depth_buffers) < self.num_reusable_buffers:
            self.reusable_depth_buffers.append(depth_buffer)

    def give_image_buffer(self, image_buffer):
        if len(self.reusable_image_buffers) < self.num_reusable_buffers:
            self.reusable_image_buffers.append(image_buffer)

    def get_width(self):
        return self.width

    def get_height(self):
        return self.height

    def get_dst_image_buffer(self):
        return self.image_buffer

    def get_dst_depth_buffer(self):
        return self.depth_buffer

    def transfer(self, depth_buffer, image_buffer):
        deepsurface.transfer(depth_buffer, image_buffer,
                             self.depth_buffer, self.image_buffer)

    def save_to_files(self, image_file_name, normals_file_name):
        self.image_buffer.save_to_file(image_file_name)
        self.depth_buffer.save_to_file(normals_file_name)
