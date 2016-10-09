__all__ = ('DeepSurface',)

from .. import deepsurface


class DeepSurface(object):
    def __init__(self, *args):
        self.src_image = deepsurface.ImageBuffer(*args)
        self.src_depth = deepsurface.DepthBuffer(*args)
        self.dst_image = deepsurface.ImageBuffer(*args)
        self.dst_depth = deepsurface.DepthBuffer(*args)

    def get_width(self):
        return self.dst_depth.get_width()

    def get_height(self):
        return self.dst_depth.get_height()

    def get_src_image_buffer(self):
        return self.src_image

    def get_src_depth_buffer(self):
        return self.src_depth

    def get_dst_image_buffer(self):
        return self.dst_image

    def get_dst_depth_buffer(self):
        return self.dst_depth

    def clear(self):
        self.src_depth.clear()
        self.src_image.clear()

    def transfer(self, and_clear=True):
        deepsurface.DeepSurface.transfer(self.src_depth, self.src_image,
                                         self.dst_depth, self.dst_image)
        if and_clear:
            self.clear()

    def save_to_files(self, image_file_name, normals_file_name):
        self.dst_image.save_to_file(image_file_name)
        self.dst_depth.save_to_file(normals_file_name)
