import unittest
from Tools.Analysis.analyze_physics_convergence import compare, interpolate


def analytic_flight(hz, error):
    # A linear six-component reference plus known first-order integration error.
    times = [0., 100., 200., 300., 400.]
    return {"path": str(hz), "hz": hz, "game_hz": 60,
            "report": {"success": True, "physical_capture": True}, "times": times,
            "samples": [tuple((i + 1) * t + error * (i + 1) for i in range(6)) for t in times]}


class ConvergenceEvidenceTests(unittest.TestCase):
    def test_interpolation_uses_requested_physical_time(self):
        self.assertEqual(interpolate(analytic_flight(120, 0), 25), (25, 50, 75, 100, 125, 150))
        with self.assertRaises(ValueError):
            interpolate(analytic_flight(120, 0), 401)

    def test_known_first_order_refinement(self):
        result = compare([analytic_flight(120, 4), analytic_flight(240, 2), analytic_flight(480, 1)])
        self.assertTrue(result["success"])
        coarse, fine = result["recovery"]["differences"]
        self.assertAlmostEqual(coarse["position_rms_m"] / fine["position_rms_m"], 2)

    def test_successful_captures_do_not_hide_diverging_trajectories(self):
        result = compare([analytic_flight(120, 0), analytic_flight(240, 1), analytic_flight(480, 5)])
        self.assertTrue(result["captures_pass"])
        self.assertFalse(result["success"])

    def test_mixed_display_rates_are_not_a_step_refinement(self):
        flights = [analytic_flight(120, 4), analytic_flight(240, 2), analytic_flight(480, 1)]
        flights[1]["game_hz"] = 30
        with self.assertRaises(ValueError):
            compare(flights)


if __name__ == "__main__":
    unittest.main()
