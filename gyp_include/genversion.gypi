##
# @internal
# @copyright © 2020, Mark Callow. For license see LICENSE.md.
#
# @brief An action for generating the version file. Include in
# a target that needs a version.
#
{
  'actions': [{
    'action_name': 'genversion',
    'inputs': [
      '../../gen-version',
      '../../.git'
    ],
    'outputs': [ 'version.h' ],
    'action': [ './gen-version' ],
  }],
}
