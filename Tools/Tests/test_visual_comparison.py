"""Prevent a render comparison from silently accepting different flight states."""
import copy
import json
import tempfile
import unittest
from pathlib import Path
from PIL import Image
from compare_visual_reviews import compare


class MovingViewComparisonTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.left, self.right = (Path(self.temp.name)/name for name in ('before', 'after'))
        for directory in (self.left, self.right):
            directory.mkdir()
            Image.new('RGB', (4, 4), 'white').save(directory/'frame.png')
        self.frame = dict(image='frame.png', phase='ENTRY', camera=0, width=4, height=4,
                          solar_hour=14, mission_time_s=317.5, world_time_s=377.5,
                          altitude_m=5000, fov_deg=55, camera_cm=[1, 2, 3],
                          body_cm=[0, 0, 500000], camera_forward=[1, 0, 0], body_up=[0, 0, 1])
        self.frame.update({'r.ScreenPercentage': 66.7, 'r.Lumen.HardwareRayTracing': 0,
                           'r.VolumetricRenderTarget.Mode': 3})
        self.save(self.left, self.frame)

    def save(self, directory, frame):
        (directory/'frames.json').write_text(json.dumps({'frames': [frame]}), encoding='utf-8')

    def test_same_flight_with_changed_render_still_requires_visual_review(self):
        changed = copy.deepcopy(self.frame)
        changed['r.VolumetricRenderTarget.Mode'] = 1
        self.save(self.right, changed)
        Image.new('RGB', (4, 4), 'black').save(self.right/'frame.png')
        result = compare(self.left, self.right)
        self.assertTrue(result['matching_views'])
        self.assertTrue(result['visual_review_required'])
        self.assertEqual(result['frames'][0]['rgb_absolute_difference_mean_255'], 255)

    def test_different_trajectory_clock_and_settings_are_rejected(self):
        for key, value in (('body_cm', [0, 0, 500100]), ('mission_time_s', 317.5333),
                           ('camera_forward', [float('nan'), 0, 0]), ('solar_hour', 15)):
            with self.subTest(key=key):
                changed = copy.deepcopy(self.frame)
                changed[key] = value
                self.save(self.right, changed)
                with self.assertRaises(ValueError):
                    compare(self.left, self.right)


if __name__ == '__main__':
    unittest.main()
