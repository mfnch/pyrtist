__all__ = ('DeepSurface',)

from .. import deepsurface


class DeepSurface(object):
    def __init__(self, width, height, num_reusable_buffers=3):
        self.width = width
        self.height = height

        self.reusable_depth_buffers = []
        self.num_reusable_buffers = num_reusable_buffers

        self.image_buffer = self.create_image_buffer()
        self.depth_buffer = self.take_depth_buffer()

    def create_image_buffer(self):
        return deepsurface.ImageBuffer(self.width, self.height)

    def take_depth_buffer(self):
        if len(self.reusable_depth_buffers) > 0:
            return self.reusable_depth_buffers.pop()
        return deepsurface.DepthBuffer(self.width, self.height)

    def give_depth_buffer(self, depth_buffer):
        if len(self.reusable_depth_buffers) < self.num_reusable_buffers:
            self.reusable_depth_buffers.append(depth_buffer)

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
