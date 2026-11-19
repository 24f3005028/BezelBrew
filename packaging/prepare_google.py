"""Prepare the publisher's Desktop OAuth identity; never accepts user tokens."""
import argparse
import json
import re
from pathlib import Path


def prepare(source: Path, destination: Path) -> None:
    if source.stat().st_size > 65536:
        raise ValueError('Choose the downloaded Desktop OAuth client JSON.')
    document = json.loads(source.read_text(encoding='utf-8-sig'))
    installed = document.get('installed', {})
    client = installed.get('client_id', '')
    secret = installed.get('client_secret', '')
    if not isinstance(client, str) or not re.fullmatch(
        r'[A-Za-z0-9._-]+\.apps\.googleusercontent\.com', client
    ) or len(client) >= 256:
        raise ValueError('The file must contain a Google Desktop OAuth client ID.')
    if not isinstance(secret, str) or not 1 <= len(secret) <= 4095 or any(
        ord(character) < 33 or ord(character) > 126 for character in secret
    ):
        raise ValueError('The Desktop OAuth client credential is missing or invalid.')
    if any(key in document or key in installed for key in ('access_token', 'refresh_token')):
        raise ValueError('User access/refresh tokens must never be bundled.')
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(
        '// Generated publisher Desktop OAuth identity. Do not commit.\n'
        f'static const wchar_t clientId[]=L{json.dumps(client)};\n'
        f'static const char clientSecret[]={json.dumps(secret)};\n',
        encoding='utf-8',
    )


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('client_json', type=Path)
    parser.add_argument('output_header', type=Path)
    arguments = parser.parse_args()
    try:
        prepare(arguments.client_json, arguments.output_header)
    except (ValueError, OSError, TypeError, AttributeError):
        parser.exit(1, 'Could not prepare Google sign-in. Use a Google Desktop client JSON without user tokens.\n')
    print('Prepared the publisher Google sign-in identity. No credentials were printed.')
