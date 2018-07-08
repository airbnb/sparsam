# Changelog

## 0.2.5

- Fix handling of top-level objects that are unions

## 0.2.4

- Include ruby headers as C in extension

## 0.2.3

- Fix base class issues for IDL defined Exceptions

## 0.2.2

- Handle eigenclasses correctly in serializer

## 0.2.1

- Fix crash when serialize is called with a mismatched type

## 0.2.0

- Move validation into serialization
- Remove field question mark methods
- Remove Ruby type checking (this is now part of serialization)

## 0.1.5

- Add structured data around where validation failed to validation errors
- Validate required fields during serialization for nested structs
- Fix segfaults on serializing objects with invalid types

## 0.1.4

- Fix for ruby 1.9

## 0.1.3

- Initial public release
