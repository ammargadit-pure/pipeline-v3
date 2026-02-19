# Shared Contracts

## Rules
1. ONLY the architect defines contracts. Developers NEVER invent new names.
2. Every identifier that a test references MUST appear in this document.
3. Only define identifiers for elements that will actually exist.

## API Response Shapes

### Success Response
````json
{
  "data": { ... },
  "meta": { "page": 1, "limit": 20, "total": 100 }
}
````

### Error Response
````json
{
  "error": "error_code",
  "message": "Human readable message",
  "details": []
}
````

### List Response
````json
{
  "data": [...],
  "total": 100,
  "page": 1,
  "limit": 20
}
````

## HTTP Status Codes
- 200: Success (GET, PUT, PATCH)
- 201: Created (POST)
- 204: No Content (DELETE)
- 400: Bad Request
- 401: Unauthorized
- 403: Forbidden
- 404: Not Found
- 422: Validation Error
- 429: Rate Limited
- 500: Internal Server Error

## Naming Conventions
# ─── CUSTOMIZE FOR YOUR STACK ───
# Example (TypeScript API):
#   Routes: kebab-case (/api/task-comments)
#   DB columns: snake_case (created_at)
#   Code variables: camelCase (createdAt)
#   Schema/Model names: PascalCase (CreateUserSchema)
# Example (Python API):
#   Routes: snake_case (/api/task_comments)
#   DB columns: snake_case (created_at)
#   Code variables: snake_case (created_at)
#   Model names: PascalCase (CreateUserRequest)
[Architect defines these for YOUR project]

## Shared Types
[Architect will define these]

## Test Data Conventions
[Architect will define these]

## Module Contracts
[Architect will define per-module contracts]
