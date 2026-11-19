"""Publisher build preparation: fake Desktop identity only; no user tokens/network."""
import importlib.util,json,tempfile,unittest
from pathlib import Path
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('prepare_google',root/'packaging'/'prepare_google.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class PublisherConfiguration(unittest.TestCase):
 def setUp(self):
  self.folder=tempfile.TemporaryDirectory(dir=root/'tools'/'tmp');self.directory=Path(self.folder.name);self.source=self.directory/'desktop.json';self.output=self.directory/'bezelbrew_google_generated.hpp'
 def tearDown(self):self.folder.cleanup()
 def write(self,document):self.source.write_text(json.dumps(document),encoding='utf-8-sig')
 def test_desktop_identity_only(self):
  self.write({'installed':{'client_id':'publisher-test.apps.googleusercontent.com','client_secret':'test-public-desktop-credential'}});module.prepare(self.source,self.output);header=self.output.read_text();self.assertIn('L"publisher-test.apps.googleusercontent.com"',header);self.assertNotIn('refresh_token',header)
 def test_web_client_is_rejected(self):
  self.write({'web':{'client_id':'publisher-test.apps.googleusercontent.com','client_secret':'test'}})
  with self.assertRaises(ValueError):module.prepare(self.source,self.output)
 def test_user_tokens_are_rejected(self):
  for key in ['refresh_token','access_token']:
   self.write({'installed':{'client_id':'publisher-test.apps.googleusercontent.com','client_secret':'test',key:'user-token-must-not-ship'}})
   with self.assertRaises(ValueError):module.prepare(self.source,self.output)
 def test_invalid_client_and_control_characters(self):
  for client,secret in [('arbitrary-host.example','test'),('publisher-test.apps.googleusercontent.com','line\nbreak')]:
   self.write({'installed':{'client_id':client,'client_secret':secret}})
   with self.assertRaises(ValueError):module.prepare(self.source,self.output)
if __name__=='__main__':unittest.main()
