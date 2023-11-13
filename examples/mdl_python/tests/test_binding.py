import unittest
import os

try:  # pragma: no cover
    # testing from within a package or CI
    from .setup import SDK, BindingModule
    from .unittest_base import UnittestBase
    pymdlsdk = BindingModule
except ImportError:  # pragma: no cover
    # local testing
    from setup import SDK
    from unittest_base import UnittestBase
    import pymdlsdk


# Test basic features of the binding logic it self
class MainBindings(UnittestBase):
    sdk: SDK = None
    tf: pymdlsdk.IType_factory = None

    @classmethod
    def setUpClass(self):
        print(f"Running tests in {__file__} in process with id: {os.getpid()}")
        self.sdk = SDK()
        self.sdk.load(addExampleSearchPath=False, loadImagePlugins=False)
        self.tf = self.sdk.mdlFactory.create_type_factory(self.sdk.transaction)

    @classmethod
    def tearDownClass(self):
        self.tf = None
        self.sdk.unload()
        self.sdk = None
        print(f"\nFinished tests in {__file__}\n")

    # Returned IInterfaces can be nullptr in C++. They are returned as invalid interfaces in python.
    # Meaning the returned object is NOT None.
    def test_with_nullptr(self):
        color: pymdlsdk.IType_color = self.tf.create_color()

        # acquiring an incompatible interface
        invalidInt: pymdlsdk.IType_int = color.get_interface(pymdlsdk.IType_int)
        self.assertIsNotNone(invalidInt)  # returned object exists
        self.assertFalse(invalidInt.is_valid_interface())  # but the interface is not valid

        # same for with-blocks, meaning you can always use a with block for returned IInterfaces
        with color.get_interface(pymdlsdk.IType_texture) as invalidTexture:
            self.assertIsNotNone(invalidTexture)  # returned object exists
            self.assertFalse(invalidTexture.is_valid_interface())  # but the interface is not valid

    # Accessing, i.e. calling, invalid IInterfaces should throw an Exception rather than crashing.
    def UPCOMING_test_nullptr_access(self):  # pragma: no cover
        invalidBool: pymdlsdk.IType_bool = self.tf.create_color().get_interface(pymdlsdk.IType_bool)
        self.assertIsNotNone(invalidBool)  # returned object exists
        self.assertFalse(invalidBool.is_valid_interface())  # but the interface is not valid
        try:
            _: pymdlsdk.IType.Kind = invalidBool.get_kind()
            self.assertTrue(False, "expected exception not fired")  # pragma: no cover
            # should not be reached
        except RuntimeError as ex:
            print(f"expected error: {ex}")
        except Exception as ex:  # pragma: no cover
            print(f"exception: {ex}")  # would catch if we do not catch `RuntimeError` before
            self.assertTrue(False, "expected exception of wrong type")  # should not be reached

    def test_manual_release(self):
        color: pymdlsdk.IType_color = self.tf.create_color()
        # UPCOMING self.assertEqual(color.__iinterface_refs__(), 1)
        self.assertTrue(color.is_valid_interface())
        color.release()  # the manual release
        self.assertFalse(color.is_valid_interface())


# run all tests of this file
if __name__ == '__main__':
    unittest.main()  # pragma: no cover
