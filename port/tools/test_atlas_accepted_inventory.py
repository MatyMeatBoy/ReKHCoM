#!/usr/bin/env python3
"""No approval may disappear or be replaced by its latest pending descendant."""
import copy
import unittest
from audit_atlas_approved_activation import validate_complete


class AcceptedInventory(unittest.TestCase):
    def setUp(self):
        self.state = {'approvals': [
            {'family_id': 'arena', 'approved_version': 'accepted', 'pending_ids': ['newer-pending']},
            {'family_id': 'scene112', 'approved_version': 'retained-acceptance'},
            {'family_id': 'unapproved', 'approved_version': None}]}
        self.report = {'accepted_total': 2, 'accepted': [
            {'family_id': 'arena', 'approved_version': 'accepted', 'current_feedback_status': 'approved'},
            {'family_id': 'scene112', 'approved_version': 'retained-acceptance',
             'current_feedback_status': 'changes_requested'}]}

    def test_every_authoritative_approval_is_retained(self):
        validate_complete(self.state, self.report)

    def test_current_feedback_is_not_a_new_approval(self):
        validate_complete(self.state, self.report)
        self.assertEqual(self.report['accepted'][1]['current_feedback_status'], 'changes_requested')

    def test_missing_row_fails(self):
        report = copy.deepcopy(self.report); report['accepted'].pop()
        with self.assertRaises(ValueError): validate_complete(self.state, report)

    def test_pending_descendant_cannot_replace_accepted_version(self):
        report = copy.deepcopy(self.report); report['accepted'][0]['approved_version'] = 'newer-pending'
        with self.assertRaises(ValueError): validate_complete(self.state, report)

    def test_duplicate_row_fails(self):
        report = copy.deepcopy(self.report); report['accepted'].append(report['accepted'][0])
        with self.assertRaises(ValueError): validate_complete(self.state, report)


if __name__ == '__main__': unittest.main()
