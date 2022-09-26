# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

"""
Utilities to parse and understand error codes.

The main entry points are 'parse_error_code' and 'error_code_map'.

Prior to using this module, the path to the error code YAML
metadata should be set using the set_error_yml_path function.
"""

import os
import yaml


def set_error_yml_path(yml_path):
    """
    Set the path containing common error code yml files.
    :param yml_path: The path containing common error codes.
    :return: None
    """
    
    global error_code_map
    yml_path=os.path.join(os.path.split(__file__)[0], yml_path)
    if not os.path.isdir(yml_path):
        print("commonerror_yml path doesn't exist")
        exit(1)        
    error_code_map = ErrorCodeMap(yml_path) 


ERROR_CATEGORY_SHIFT = 20
ERROR_CODE_MASK = (1 << ERROR_CATEGORY_SHIFT) - 1


def get_category(code):
    """
    Gets the error category from the specified full error code.
    :param code: An common error code as an integer.
    :return: The category number of the code.
    """
    return code >> ERROR_CATEGORY_SHIFT


def get_id(code):
    """
    Gets the category-relative ID of the specified common full error code.
    :param code: An common error code as an integer.
    :return: The category-relative ID of the error code.
    """
    return code & ERROR_CODE_MASK


def get_full_code(category, code):
    """
    Constructs an common integer error code out of a category ID and a category-relative
    code.
    :param category: An common error category ID.
    :param code: An common category-relative code.
    :return: The full common error code.
    """
    return (category << ERROR_CATEGORY_SHIFT) | (code & ERROR_CODE_MASK)
    return (category << ERROR_CATEGORY_SHIFT) | (code & ERROR_CODE_MASK)


class ErrorCode:
    """
    Represents a parsed error code.
    """

    def __init__(self, code=0, name='CommonError::Success', msg='Success', is_untranslated=False):
        self.code = code
        self.name = name
        self.message = msg
        self._is_untranslated = is_untranslated

    def __str__(self):
        return "0x%x" % self.code if self._is_untranslated else "%s" % self.name

    def __repr__(self):
        return str(vars(self))

    @property
    def category(self):
        """The category ID of the error code."""
        return get_category(self.code)

    @property
    def id(self):
        """The category-relative error code ID."""
        return get_id(self.code)

    @property
    def is_untranslated(self):
        """True if the error code doesn't belong to any category."""
        return self._is_untranslated


class ErrorCodeMap:
    """
    Class to lazily load and parse error codes from the yaml files.
    """
    def __init__(self, yml_path=None):
        self.__error_codes = None
        self.__categories = None
        self.__error_yml_path = yml_path

    def __get_error_map(self):
        if not self.__error_codes or not self.__categories:
            self.__error_codes = {}
            self.__categories = {}
            yml_path = self.__error_yml_path
            for yml in os.listdir(yml_path):
                if not yml.endswith('.yml'):
                    continue
                with open(os.path.join(yml_path, yml)) as stream:
                    data = yaml.safe_load(stream)
                    category_name = data['name']
                    category_id = data['category_id']
                    category = {'id': category_id,
                                'name': category_name}
                    category_codes = {}
                    self.__categories[category_id] = {'name': category_name}
                    if 'codes' in data:
                        for code in data['codes']:
                            full_code = get_full_code(category_id, code['id'])
                            name = '%s::%s' % (category_name, code['name'])
                            error_code = ErrorCode(full_code, name, code['msg'])
                            category_codes[full_code] = error_code
                            self.__error_codes[full_code] = error_code

                    category['codes'] = category_codes
                    self.__categories[category_id] = category
            self.__error_codes[0] = ErrorCode()

        return self.__error_codes, self.__categories

    @property
    def categories(self):
        """A map of error code category objects, with category IDs as keys"""
        return self.__get_error_map()[1]

    @property
    def all_codes(self):
        """A map of error codes, with the full error code as keys"""
        return self.__get_error_map()[0]

    def get_error_code(self, code):
        error_map, categories = self.__get_error_map()
        err = error_map.get(code, None)
        if not err:
            category = categories.get(get_category(code), None)
            if category:
                category_name = category['name']
                id = get_id(code)
                err = ErrorCode(code, name="%s::%d" % (category_name, id),
                                msg="%s error code: %d" % (category_name, id))
        if not err:
            err = ErrorCode(code, "UnknownError", "<Error Translation failed>", is_untranslated=True)

        return err


error_code_map = ErrorCodeMap()


def parse_error_code(error_code):
    """
    Returns an ErrorCode object for the specified error code.
    :param error_code: A string or integer error code value.
    :return: An ErrorCode object.
    """
    if isinstance(error_code, str):
        if error_code.count(':') == 1:
            category, code = (int(x, 0) for x in error_code.split(":"))
            error_code = get_full_code(category, code)
        else:
            error_code = int(error_code, 0)

    return error_code_map.get_error_code(error_code)
