"""Independent fixtures for tail latency, CSV filtering and missing counters."""
import tempfile
import unittest
from pathlib import Path
from analyze_performance import analyze


class PerformanceAnalysisTest(unittest.TestCase):
    def capture(self, content, warmup=0):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'fixture.csv'
            path.write_text(content,encoding='utf-8')
            return analyze(path,warmup)

    def test_tail_latency_and_phase_coverage(self):
        result=self.capture('FrameTime,Recovery/Phase,Recovery/MissionTimeS,GPUMem/LocalUsedMB\n'
                            '10,1,-1,500\n10,2,0,600\n10,2,1,650\n100,9,400,700\n')
        self.assertEqual(result['fps_from_mean'],30.77)
        self.assertEqual(result['fps_from_worst_one_percent_mean'],10)
        self.assertAlmostEqual(result['timings']['FrameTime']['p99_ms'],97.3)
        self.assertEqual(result['phases']['Captured']['frames_measured'],1)
        self.assertEqual(result['memory']['GPUMem/LocalUsedMB']['max_mb'],700)
        self.assertEqual(result['coverage']['Recovery/MissionTimeS']['min'],-1)
        self.assertEqual(result['slowest_frames'][0]['valid_frame_index'],3)

    def test_metadata_missing_and_startup_are_explicit(self):
        result=self.capture('FrameTime,GPUTime\n1000,900\n10,\n20,12\nmetadata,\nnan,0\n',1)
        self.assertEqual(result['frames_measured'],2)
        self.assertEqual(result['metadata_or_invalid_rows'],2)
        self.assertEqual(result['timings']['GPUTime']['samples'],1)
        self.assertEqual(result['startup']['timings']['FrameTime']['max_ms'],1000)
        self.assertEqual(result['memory'],{})
        self.assertEqual(result['phases'],{})

    def test_short_or_wrong_capture_is_rejected(self):
        with self.assertRaises(ValueError):self.capture('FrameTime\n10\n',300)
        with self.assertRaises(ValueError):self.capture('Altitude\n100\n')
        with self.assertRaises(ValueError):self.capture('FrameTime\n10\n',-1)

    def test_unavailable_engine_counters_are_not_negative_totals(self):
        result=self.capture('FrameTime,PSO/PSOMissesOnHitch,GPUMem/LocalUsedMB\n10,-1,-1\n20,-1,-1\n')
        self.assertEqual(result['pso_counters'],{})
        self.assertEqual(result['memory'],{})
        self.assertIn('PSO/PSOMissesOnHitch',result['unavailable_counters'])


if __name__=='__main__':unittest.main()
